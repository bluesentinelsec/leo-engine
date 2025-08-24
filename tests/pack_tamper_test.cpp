#include <catch2/catch_all.hpp>
#include <leo/pack_errors.h>
#include <leo/pack_format.h>
#include <leo/pack_reader.h>
#include <leo/pack_util.h>
#include <leo/pack_writer.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static fs::path out_path(const char* name)
{
	return fs::temp_directory_path() / name;
}

static void write_all(const fs::path& p, const std::vector<uint8_t>& bytes)
{
	std::ofstream o(p, std::ios::binary | std::ios::trunc);
	o.write((const char*)bytes.data(), (std::streamsize)bytes.size());
}

static std::vector<uint8_t> read_all(const fs::path& p)
{
	std::ifstream i(p, std::ios::binary);
	std::vector<uint8_t> v((std::istreambuf_iterator<char>(i)), {});
	return v;
}

TEST_CASE("tamper: header CRC mismatch -> E_FORMAT on open")
{
	fs::path outP = out_path("tamper_hdr.leopack");
	std::error_code ec;
	fs::remove(outP, ec);

	// Build a minimal pack
	leo_pack_writer* w = nullptr;
	leo_pack_build_opts opt{};
	REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);
	const char d[] = "x";
	REQUIRE(leo_pack_writer_add(w, "x.bin", d, 1, 0, 0) == LEO_PACK_OK);
	REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

	// Flip a non-CRC byte in header and DO NOT update header_crc32
	auto bytes = read_all(outP);
	REQUIRE(bytes.size() > sizeof(leo_pack_header_v1));
	bytes[4] ^= 0xFF; // inside header but not in the crc field (last 4 bytes)
	write_all(outP, bytes);

	leo_pack* p = nullptr;
	REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_E_FORMAT);
}

TEST_CASE("tamper: shrink TOC size -> E_FORMAT during TOC parse")
{
	fs::path outP = out_path("tamper_toc.leopack");
	std::error_code ec;
	fs::remove(outP, ec);

	// Build with one entry
	leo_pack_writer* w = nullptr;
	leo_pack_build_opts opt{};
	REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);
	const char d[] = "y";
	REQUIRE(leo_pack_writer_add(w, "y.bin", d, 1, 0, 0) == LEO_PACK_OK);
	REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

	// Read header, decrement toc_size by 1, recompute header_crc32, rewrite header
	std::fstream f(outP, std::ios::binary | std::ios::in | std::ios::out);
	leo_pack_header_v1 hdr{};
	f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
	hdr.toc_size -= 1;

	// recompute header CRC over the struct with header_crc32 zeroed
	uint8_t tmp[sizeof(hdr)];
	memcpy(tmp, &hdr, sizeof(hdr));
	memset(tmp + offsetof(leo_pack_header_v1, header_crc32), 0, sizeof(uint32_t));

	hdr.header_crc32 = leo_crc32_ieee(tmp, sizeof(hdr), 0);

	f.seekp(0);
	f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
	f.close();

	// Open should now fail while parsing TOC
	leo_pack* p = nullptr;
	REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_E_FORMAT);
}

TEST_CASE("tamper: corrupt stored payload -> DECOMPRESS or BAD_PASSWORD")
{
	fs::path outP = out_path("tamper_payload.leopack");
	std::error_code ec;
	fs::remove(outP, ec);

	// Obfuscated + compressed entry so both code paths are exercised
	const std::string big(1024, 'A');
	leo_pack_writer* w = nullptr;
	leo_pack_build_opts opt{};
	opt.password = "pw";
	opt.level = 7;
	REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);
	REQUIRE(leo_pack_writer_add(w, "bin.dat", big.data(), big.size(), 1, 1) == LEO_PACK_OK);
	REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

	// Locate the stored payload offset by parsing TOC (simple parser inline)
	std::ifstream in(outP, std::ios::binary);
	leo_pack_header_v1 hdr{};
	in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));

	// Seek to TOC and read name_len + name + meta
	in.seekg((std::streamoff)hdr.toc_offset);
	uint16_t nlen = 0;
	in.read(reinterpret_cast<char*>(&nlen), sizeof(nlen));
	std::vector<char> name(nlen + 1);
	if (nlen)
		in.read(name.data(), nlen);
	name[nlen] = '\0';
	leo_pack_entry_v1 meta{};
	in.read(reinterpret_cast<char*>(&meta), sizeof(meta));
	in.close();

	// Flip a byte inside stored payload (guaranteed within range)
	auto bytes = read_all(outP);
	size_t off = (size_t)meta.offset + meta.size_stored / 2;
	REQUIRE(off < bytes.size());
	bytes[off] ^= 0x80;
	write_all(outP, bytes);

	// With correct password, extraction should fail. If decompression fails early, E_BAD_PASSWORD is returned
	// for obfuscated entries on error; otherwise, CRC mismatch maps to E_BAD_PASSWORD too.
	leo_pack* p = nullptr;
	REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), "pw") == LEO_PACK_OK);
	int idx = -1;
	REQUIRE(leo_pack_find(p, "bin.dat", &idx) == LEO_PACK_OK);
	std::vector<char> out(meta.size_uncompressed);
	size_t got = 0;
	REQUIRE(leo_pack_extract_index(p, idx, out.data(), out.size(), &got) == LEO_PACK_E_BAD_PASSWORD);
	leo_pack_close(p);
}
