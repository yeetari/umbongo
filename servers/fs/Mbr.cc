struct [[gnu::packed]] MbrEntry {
    uint8 status;
    ustd::Array<uint8, 3> chs1;
    uint8 type;
    ustd::Array<uint8, 3> chs2;
    uint32 lba;
    uint32 length;
};

ustd::Array<uint8, 512> first_sector{};
EXPECT(disk.read(first_sector.span(), 0));
ENSURE(first_sector[510] == 0x55);
ENSURE(first_sector[511] == 0xaa);

for (usize i = 0; i < 4; i++) {
    auto &entry = *reinterpret_cast<MbrEntry *>(&first_sector[446 + i * sizeof(MbrEntry)]);
    if (entry.lba == 0) {
        continue;
    }
    log::info("start={:h} end={:h}", entry.lba, entry.lba + entry.length);
    log::info("start={:h} end={:h}", entry.lba * 512, (entry.lba + entry.length) * 512);
}
