#include "unity.h"

// Use relative path
#include "../include/common.h"

// Test _reverse_bits function
TEST_CASE("Reverse bits - 0x00", "[common]")
{
    TEST_ASSERT_EQUAL_UINT8(0x00, _reverse_bits(0x00));
}

TEST_CASE("Reverse bits - 0xFF", "[common]")
{
    TEST_ASSERT_EQUAL_UINT8(0xFF, _reverse_bits(0xFF));
}

TEST_CASE("Reverse bits - 0x01 to 0x80", "[common]")
{
    // 0x01 = 0b00000001 -> 0b10000000 = 0x80
    TEST_ASSERT_EQUAL_UINT8(0x80, _reverse_bits(0x01));
}

TEST_CASE("Reverse bits - 0x80 to 0x01", "[common]")
{
    // 0x80 = 0b10000000 -> 0b00000001 = 0x01
    TEST_ASSERT_EQUAL_UINT8(0x01, _reverse_bits(0x80));
}

TEST_CASE("Reverse bits - 0xAA to 0x55", "[common]")
{
    // 0xAA = 0b10101010 -> 0b01010101 = 0x55
    TEST_ASSERT_EQUAL_UINT8(0x55, _reverse_bits(0xAA));
}

TEST_CASE("Reverse bits - 0x55 to 0xAA", "[common]")
{
    // 0x55 = 0b01010101 -> 0b10101010 = 0xAA
    TEST_ASSERT_EQUAL_UINT8(0xAA, _reverse_bits(0x55));
}

TEST_CASE("Reverse bits - 0x0F to 0xF0", "[common]")
{
    // 0x0F = 0b00001111 -> 0b11110000 = 0xF0
    TEST_ASSERT_EQUAL_UINT8(0xF0, _reverse_bits(0x0F));
}

TEST_CASE("Reverse bits - 0x12 to 0x48", "[common]")
{
    // 0x12 = 0b00010010 -> 0b01001000 = 0x48
    TEST_ASSERT_EQUAL_UINT8(0x48, _reverse_bits(0x12));
}

TEST_CASE("Reverse bits - double reverse returns original", "[common]")
{
    uint8_t original = 0x37;
    TEST_ASSERT_EQUAL_UINT8(original, _reverse_bits(_reverse_bits(original)));
}

// Test _largest_power_of_two function
TEST_CASE("Largest power of two - 1", "[common]")
{
    TEST_ASSERT_EQUAL_INT(1, _largest_power_of_two(1));
}

TEST_CASE("Largest power of two - 2", "[common]")
{
    TEST_ASSERT_EQUAL_INT(2, _largest_power_of_two(2));
}

TEST_CASE("Largest power of two - 3", "[common]")
{
    // Largest power of 2 <= 3 is 2
    TEST_ASSERT_EQUAL_INT(2, _largest_power_of_two(3));
}

TEST_CASE("Largest power of two - 4", "[common]")
{
    TEST_ASSERT_EQUAL_INT(4, _largest_power_of_two(4));
}

TEST_CASE("Largest power of two - 5", "[common]")
{
    // Largest power of 2 <= 5 is 4
    TEST_ASSERT_EQUAL_INT(4, _largest_power_of_two(5));
}

TEST_CASE("Largest power of two - 7", "[common]")
{
    // Largest power of 2 <= 7 is 4
    TEST_ASSERT_EQUAL_INT(4, _largest_power_of_two(7));
}

TEST_CASE("Largest power of two - 8", "[common]")
{
    TEST_ASSERT_EQUAL_INT(8, _largest_power_of_two(8));
}

TEST_CASE("Largest power of two - 256", "[common]")
{
    TEST_ASSERT_EQUAL_INT(256, _largest_power_of_two(256));
}

TEST_CASE("Largest power of two - 257", "[common]")
{
    // Largest power of 2 <= 257 is 256
    TEST_ASSERT_EQUAL_INT(256, _largest_power_of_two(257));
}

TEST_CASE("Largest power of two - 1000", "[common]")
{
    // Largest power of 2 <= 1000 is 512
    TEST_ASSERT_EQUAL_INT(512, _largest_power_of_two(1000));
}

TEST_CASE("Largest power of two - 1024", "[common]")
{
    TEST_ASSERT_EQUAL_INT(1024, _largest_power_of_two(1024));
}

TEST_CASE("Largest power of two - 65535", "[common]")
{
    // Largest power of 2 <= 65535 is 32768
    TEST_ASSERT_EQUAL_INT(32768, _largest_power_of_two(65535));
}

// Test get_difficulty_mask function
TEST_CASE("Difficulty mask - difficulty 1", "[common]")
{
    uint8_t mask[6];
    get_difficulty_mask(1, mask);
    
    // Difficulty 1: _largest_power_of_two(1) - 1 = 0
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[4]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[5]); // reversed 0x00
}

TEST_CASE("Difficulty mask - difficulty 256", "[common]")
{
    uint8_t mask[6];
    get_difficulty_mask(256, mask);
    
    // Difficulty 256: _largest_power_of_two(256) - 1 = 255 = 0x000000FF
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[4]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0xFF, mask[5]); // reversed 0xFF
}

TEST_CASE("Difficulty mask - difficulty 512", "[common]")
{
    uint8_t mask[6];
    get_difficulty_mask(512, mask);
    
    // Difficulty 512: _largest_power_of_two(512) - 1 = 511 = 0x000001FF
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x80, mask[4]); // reversed 0x01 = 0x80
    TEST_ASSERT_EQUAL_UINT8(0xFF, mask[5]); // reversed 0xFF = 0xFF
}

TEST_CASE("Difficulty mask - difficulty 1024", "[common]")
{
    uint8_t mask[6];
    get_difficulty_mask(1024, mask);
    
    // Difficulty 1024: _largest_power_of_two(1024) - 1 = 1023 = 0x000003FF
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0xC0, mask[4]); // reversed 0x03 = 0xC0
    TEST_ASSERT_EQUAL_UINT8(0xFF, mask[5]); // reversed 0xFF = 0xFF
}

TEST_CASE("Difficulty mask - difficulty 2048", "[common]")
{
    uint8_t mask[6];
    get_difficulty_mask(2048, mask);
    
    // Difficulty 2048: _largest_power_of_two(2048) - 1 = 2047 = 0x000007FF
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0xE0, mask[4]); // reversed 0x07 = 0xE0
    TEST_ASSERT_EQUAL_UINT8(0xFF, mask[5]); // reversed 0xFF = 0xFF
}

TEST_CASE("Difficulty mask - non-power-of-two reduced to power", "[common]")
{
    uint8_t mask[6];
    // Difficulty 300: _largest_power_of_two(300) = 256, so 256 - 1 = 255 = 0x000000FF
    // Same as difficulty 256
    get_difficulty_mask(300, mask);
    
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[0]); // Fixed
    TEST_ASSERT_EQUAL_UINT8(0x14, mask[1]); // TICKET_MASK
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[2]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[3]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, mask[4]); // reversed 0x00
    TEST_ASSERT_EQUAL_UINT8(0xFF, mask[5]); // reversed 0xFF
}
