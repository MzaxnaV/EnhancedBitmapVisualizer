#pragma once
#include <cstdint>

#pragma pack(push, 1)

// 'h','h','a','f' on little-endian
static constexpr uint32_t HHA_MAGIC_VALUE = 0x66616868u;
static constexpr uint32_t HHA_VERSION     = 0;

enum class AssetTypeID : uint32_t {
    Asset_NONE = 0,
    Asset_Test_Bitmap,
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
    Asset_Rock,
    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,
    Asset_Head,
    Asset_Cape,
    Asset_Torso,
    Asset_Font,
    Asset_FontGlyph,
    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
    Asset_test_stereo,
    Asset_COUNT
};

enum class SoundChain : uint32_t {
    None    = 0,
    Loop    = 1,
    Advance = 2,
};

struct HHAHeader {
    uint32_t magicValue;
    uint32_t version;
    uint32_t tagCount;
    uint32_t assetTypeCount;
    uint32_t assetCount;
    uint64_t tagsOffset;       // absolute file offset to hha_tag[tagCount]
    uint64_t assetTypesOffset; // absolute file offset to hha_asset_type[assetTypeCount]
    uint64_t assetsOffset;     // absolute file offset to hha_asset[assetCount]
};
static_assert(sizeof(HHAHeader) == 44);

struct HHATag {
    uint32_t ID;
    float    value;
};
static_assert(sizeof(HHATag) == 8);

struct HHAAssetType {
    uint32_t typeID;
    uint32_t firstAssetIndex;
    uint32_t onePastLastAssetIndex;
};
static_assert(sizeof(HHAAssetType) == 12);

struct HHABitmap {
    uint32_t dim[2];             // [width, height]
    float    alignPercentage[2];
};
static_assert(sizeof(HHABitmap) == 16);

struct HHASound {
    uint32_t   sampleCount;
    uint32_t   channelCount;
    SoundChain chain;
};
static_assert(sizeof(HHASound) == 12);

struct HHAFont {
    uint32_t onePastHighestCodepoint;
    uint32_t glyphCount;
    float    descenderHeight;
    float    ascenderHeight;
    float    externalLeading;
};
static_assert(sizeof(HHAFont) == 20);

struct HHAFontGlyph {
    uint32_t unicodeCodePoint;
    uint32_t bitmapID;
};
static_assert(sizeof(HHAFontGlyph) == 8);

union HHAAssetData {
    HHABitmap bitmap;
    HHASound  sound;
    HHAFont   font;
};
static_assert(sizeof(HHAAssetData) == 20); // sized to largest member (HHAFont)

struct HHAAsset {
    uint64_t     dataOffset;
    uint32_t     firstTagIndex;
    uint32_t     onePastLastTagIndex;
    HHAAssetData data;
};
static_assert(sizeof(HHAAsset) == 36);

#pragma pack(pop)

// Helpers
inline bool isBitmapType(AssetTypeID id) {
    return id >= AssetTypeID::Asset_Test_Bitmap && id <= AssetTypeID::Asset_Torso;
}
inline bool isSoundType(AssetTypeID id) {
    return id >= AssetTypeID::Asset_Bloop && id <= AssetTypeID::Asset_test_stereo;
}
inline bool isFontType(AssetTypeID id)      { return id == AssetTypeID::Asset_Font; }
inline bool isFontGlyphType(AssetTypeID id) { return id == AssetTypeID::Asset_FontGlyph; }