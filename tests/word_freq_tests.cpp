#include "ChunkProcessor.hpp"
#include "sha256.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
int Failures = 0;

void Expect(bool Condition, const char* Message)
{
    if (!Condition)
    {
        std::cerr << "FAIL: " << Message << "\n";
        ++Failures;
    }
}

std::string Sha256Hex(const std::string& Data)
{
    SHA256_CTX Ctx;
    sha256_init(&Ctx);
    sha256_update(&Ctx, reinterpret_cast<const uint8_t*>(Data.data()), Data.size());

    uint8_t Hash[32];
    sha256_final(&Ctx, Hash);

    static const char* HexDigits = "0123456789abcdef";
    std::string Result;
    Result.reserve(64);
    for (uint8_t Byte : Hash)
    {
        Result.push_back(HexDigits[Byte >> 4]);
        Result.push_back(HexDigits[Byte & 0x0F]);
    }
    return Result;
}

using CountMap = std::unordered_map<std::string, uint64_t>;

void MergeInto(CountMap& Target, const CountMap& Source)
{
    for (const auto& Entry : Source)
    {
        Target[Entry.first] += Entry.second;
    }
}

void TestSha256()
{
    Expect(Sha256Hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
           "SHA-256 of empty string");
    Expect(Sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
           "SHA-256 of 'abc'");
    Expect(Sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
               "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
           "SHA-256 of multi-block input");
    Expect(Sha256Hex(std::string(1000000, 'a')) ==
               "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
           "SHA-256 of 1,000,000 'a' characters");

    const std::string Data = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    SHA256_CTX Whole;
    sha256_init(&Whole);
    sha256_update(&Whole, reinterpret_cast<const uint8_t*>(Data.data()), Data.size());
    uint8_t WholeHash[32];
    sha256_final(&Whole, WholeHash);

    SHA256_CTX Split;
    sha256_init(&Split);
    sha256_update(&Split, reinterpret_cast<const uint8_t*>(Data.data()), 1);
    sha256_update(&Split, reinterpret_cast<const uint8_t*>(Data.data() + 1), 10);
    sha256_update(&Split, reinterpret_cast<const uint8_t*>(Data.data() + 11), Data.size() - 11);
    uint8_t SplitHash[32];
    sha256_final(&Split, SplitHash);

    Expect(std::memcmp(WholeHash, SplitHash, 32) == 0, "SHA-256 split updates match whole input");
}

void TestChunkBoundaries()
{
    const std::vector<std::string> Inputs = {
        "hello world hello world",
        "a b c d e f g h i j",
        "one two three four five six seven eight nine ten",
        "splitwordhereandmore",
        " short  long\twords mixed \n delimiters ",
        std::string("cps \xC0\xE0\xC1\xE1\xA8\xB8 end")
    };

    for (const std::string& Input : Inputs)
    {
        const CountMap Whole = ChunkProcessor::Process(Input.data(), Input.size(), 0, Input.size());
        for (size_t Split = 1; Split < Input.size(); ++Split)
        {
            CountMap SplitMerged;
            MergeInto(SplitMerged, ChunkProcessor::Process(Input.data(), Input.size(), 0, Split));
            MergeInto(SplitMerged, ChunkProcessor::Process(Input.data(), Input.size(), Split, Input.size()));
            Expect(SplitMerged == Whole, "two-chunk split must match whole input");
        }
    }

    Expect(ChunkProcessor::Process("", 0, 0, 0).empty(), "empty input produces empty map");
    Expect(ChunkProcessor::Process("single", 6, 0, 6).empty(), "single word produces empty map");
}
}

int main()
{
    TestSha256();
    TestChunkBoundaries();

    if (Failures != 0)
    {
        std::cerr << Failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
