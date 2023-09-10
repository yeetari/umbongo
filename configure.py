#!/usr/bin/env python3

from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from tools.ninja_syntax import Writer as NinjaWriter
import argparse
import sys
import tomllib


class TargetKind(Enum):
    Phony = 'phony'
    Command = 'command'
    Executable = 'executable'
    StaticLibrary = 'static_library'
    ObjectLibrary = 'object_library'


@dataclass
class Target:
    name: str
    kind: TargetKind
    dir_path: Path
    output: str
    deps: list[str]
    sources: list[str]
    flags: dict[str, str]
    uses_console: bool
    commands: list[str]


def add_flags(flags: dict[str, str], obj):
    if 'asm_flags' in obj:
        flags['asm'] += ' ' + obj['asm_flags']
    if 'cxx_flags' in obj:
        flags['cxx'] += ' ' + obj['cxx_flags']
    if 'ld_flags' in obj:
        flags['ld'] += ' ' + obj['ld_flags']


def build_target(definition, dir_path: Path, flags: dict[str, str], sources: list[str], kind: TargetKind | None) -> Target:
    if kind is None:
        kind = {
            'static': TargetKind.StaticLibrary,
            'object': TargetKind.ObjectLibrary,
        }[definition.get('type', 'static')]

    name = definition.get('name', None) or definition['output']
    if kind == TargetKind.StaticLibrary:
        output = str(dir_path.joinpath('lib' + name).with_suffix('.a'))
    else:
        output = str(dir_path.joinpath(name))

    commands = []
    if kind == TargetKind.Command:
        commands.append(definition['command'])
    elif kind == TargetKind.Phony:
        commands.extend(definition.get('commands', []))

    if definition.get('no_inherit', False):
        flags = {'cxx': '', 'ld': ''}

    # Add flags for this target.
    add_flags(flags, definition)

    return Target(
        name=name,
        kind=kind,
        dir_path=dir_path,
        output=output,
        deps=definition.get('deps', []),
        sources=sources + definition.get('sources', []),
        flags=flags,
        uses_console=definition.get('uses_console', False),
        commands=commands,
    )


class Context(NinjaWriter):
    source_root: Path
    build_root: Path
    preset: str

    toml_paths: list[str] = []
    targets: list[Target] = []

    def __init__(self, source_root, build_root, preset):
        self.source_root = source_root
        self.build_root = build_root
        self.preset = preset

        self.build_root.mkdir(exist_ok=True)
        output_path = self.build_root.joinpath('build.ninja')
        output_file = output_path.open('w')
        super().__init__(output_file, width=120)

    def build_tree(self, dir_path: Path, flags: dict[str, str]):
        # Add toml path to list for the auto-reconfigure rule.
        toml_path = dir_path.joinpath('build.toml')
        self.toml_paths.append(f'$root/{str(toml_path)}')

        # Load and parse toml.
        print(f'Building {str(toml_path)}')
        with self.source_root.joinpath(toml_path).open('rb') as file:
            toml = tomllib.load(file)

        # Add flags global to this toml file.
        add_flags(flags, toml)

        # Add flags for the current preset.
        presets = toml.get('preset', {})
        if presets:
            if dir_path != Path('.'):
                print('Preset in non-root build.toml!')
                sys.exit(1)
            if self.preset not in presets:
                print(f'No preset {self.preset}')
                sys.exit(1)
            add_flags(flags, presets[self.preset])

        # Recurse through subdirectories.
        for subdir in toml.get('subdirs', []):
            self.build_tree(dir_path.joinpath(subdir), flags.copy())

        # Get sources global to each target in this toml file.
        sources = toml.get('sources', [])

        # Create targets.
        for command in toml.get('command', []):
            self.targets.append(build_target(command, dir_path, {}, [], TargetKind.Command))
        for executable in toml.get('executable', []):
            self.targets.append(build_target(executable, dir_path, flags.copy(), sources, TargetKind.Executable))
        for library in toml.get('library', []):
            self.targets.append(build_target(library, dir_path, flags.copy(), sources, None))
        for phony in toml.get('phony', []):
            self.targets.append(build_target(phony, dir_path, {}, [], TargetKind.Phony))

    def generate_target(self, target: Target):
        self.comment(target.name)

        explicit_dependencies = []
        implicit_dependencies = []
        order_only_dependencies = []
        deps = [t for t in self.targets if t.name in target.deps]
        for dep in deps:
            if dep.kind != TargetKind.ObjectLibrary:
                implicit_dependencies.append(dep.output)
                continue

            order_only_dependencies.append(dep.output)
            for dep_source in dep.sources:
                # TODO: Duplicated.
                object_path = dep.dir_path.joinpath(dep.name + '-objs').joinpath(dep_source + '.o')
                explicit_dependencies.append(str(object_path))

        for source in target.sources:
            object_path = target.dir_path.joinpath(target.name + '-objs').joinpath(source + '.o')
            explicit_dependencies.append(str(object_path))

            source_path = target.dir_path.joinpath(source)
            source_type = {
                '.asm': 'asm',
                '.cc': 'cxx',
            }[str(source_path.suffix)]

            variables = {}
            flags = target.flags[source_type].strip()
            if flags != '':
                variables['flags'] = flags

            root_path = '$root/'
            for t in self.targets:
                if str(source_path) == t.output:
                    root_path = ''

            self.build(outputs=str(object_path),
                       rule=source_type,
                       inputs=root_path + str(source_path),
                       variables=variables)

        rule = {
            TargetKind.Phony: 'phony',
            TargetKind.Command: 'custom_command',
            TargetKind.Executable: 'link',
            TargetKind.StaticLibrary: 'link-static',
            TargetKind.ObjectLibrary: 'phony',
        }[target.kind]

        variables = {}
        if target.kind == TargetKind.Command:
            variables['command'] = target.commands[0]
            variables['description'] = f'Building {target.output}'
        elif target.kind == TargetKind.Executable:
            variables['flags'] = target.flags['cxx'] + ' ' + target.flags['ld']
            variables['libs'] = ''

            for dep in filter(lambda d: d.kind == TargetKind.StaticLibrary, deps):
                variables['libs'] += dep.output + ' '

        pool = 'console' if target.uses_console else None

        # Build command target.
        if target.kind == TargetKind.Phony and len(target.commands) != 0:
            command_target = f'{target.name}_commands'

            concatenated_commands = ''
            for i, command in enumerate(target.commands):
                if i != 0:
                    concatenated_commands += ' && '
                concatenated_commands += command

            self.newline()
            self.build(outputs=command_target,
                       rule='custom_command',
                       order_only=explicit_dependencies + implicit_dependencies + order_only_dependencies,
                       variables={
                           'command': concatenated_commands,
                           'description': f'Commands for phony target {target.name}',
                       },
                       pool=pool)
            explicit_dependencies.append(command_target)

        self.newline()
        self.build(outputs=target.output,
                   rule=rule,
                   inputs=explicit_dependencies,
                   implicit=implicit_dependencies,
                   order_only=order_only_dependencies,
                   variables=variables,
                   pool=pool)
        self.newline()

    def generate_targets(self):
        for target in self.targets:
            self.generate_target(target)


def main():
    arg_parser = argparse.ArgumentParser(prog='Configure', description='Configure umbongo build')
    arg_parser.add_argument('-B', default='build', dest='build_dir')
    arg_parser.add_argument('-p', '--preset', default='debug')
    args = arg_parser.parse_args()

    context = Context(
        source_root=Path(__file__).parent,
        build_root=Path(args.build_dir),
        preset=args.preset,
    )
    context.build_tree(Path('.'), {
        'asm': '',
        'cxx': '',
        'ld': '',
    })

    context.targets.append(Target(
        name='all',
        kind=TargetKind.Phony,
        dir_path=Path('.'),
        output='all',
        deps=[target.name for target in context.targets
              if target.kind not in [TargetKind.Phony, TargetKind.ObjectLibrary]],
        sources=[],
        flags={},
        uses_console=False,
        commands=[],
    ))

    context.comment('Generated by umbongo\'s configure.py')
    context.variable('ninja_required_version', '1.11')
    context.newline()

    context.variable('build_preset', context.preset)
    context.variable('build_root', str(context.build_root.absolute()))
    context.variable('root', str(context.source_root))
    context.newline()

    context.comment('Rules')
    context.rule('clean', '/usr/bin/ninja -t clean')
    context.newline()
    context.rule('custom_command',
                 '$command',
                 description='$description')
    context.newline()
    context.rule('asm',
                 'nasm -MD $out.d -MT $out -f elf64 -o $out $in',
                 depfile='$out.d',
                 deps='gcc',
                 description='Building ASM object $out')
    context.newline()
    context.rule('cxx',
                 'clang++ $flags -MD -MT $out -MF $out.d -o $out -c $in',
                 depfile='$out.d',
                 deps='gcc',
                 description='Building CXX object $out')
    context.newline()
    context.rule('link',
                 'clang++ $flags $in -o $out $libs',
                 description='Linking executable $out')
    context.newline()
    context.rule('link-static',
                 'rm -f $out && llvm-ar qcs $out $in',
                 description='Linking static library $out')
    context.newline()

    # Write ninja for targets.
    context.generate_targets()

    # Write auto-reconfigure rule.
    context.comment('Regenerate build files if python or toml changes.')
    context.rule('configure',
                 command='$root/configure.py -B $build_root -p $build_preset',
                 description='Reconfiguring...',
                 generator=1)
    context.build('build.ninja', 'configure', pool='console',
                  implicit=['$root/configure.py', '$root/tools/ninja_syntax.py'] + context.toml_paths)
    context.newline()

    context.build('clean', 'clean')
    context.default('all')
    context.close()


if __name__ == '__main__':
    main()
