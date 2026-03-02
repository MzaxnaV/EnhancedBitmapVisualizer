#include <hex/plugin.hpp>
#include <hex/api/content_registry/pattern_language.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/ui/view.hpp>
#include <pl/patterns/pattern.hpp>
#include <imgui.h>
#include <hex/ui/imgui_imhex_extensions.h>
#include <hex/providers/provider.hpp>
// #include <hex/api/imhex_api.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include "hha.h"

// ─── Bitmap Visualizer ────────────────────────────────────────────────────────

void drawCustomPackedBitmapVisualizer(pl::ptrn::Pattern &, bool shouldReset, std::span<const pl::core::Token::Literal> arguments)
{
    static ImGuiExt::Texture texture;
    static float scale = 1.0F;

    if (shouldReset)
    {
        auto pattern = arguments[0].toPattern();
        auto width = arguments[1].toUnsigned();
        auto height = arguments[2].toUnsigned();
        auto order = arguments[3].toUnsigned();

        auto data = pattern->getBytes();
        size_t size = data.size();
        std::vector<u8> convertedData(size);

        u8 rOrder = (order >> 24) & 0xFF;
        u8 gOrder = (order >> 16) & 0xFF;
        u8 bOrder = (order >> 8) & 0xFF;
        u8 aOrder = order & 0xFF;

        for (size_t i = 0; i < size; i += 4)
        {
            convertedData[i + 0] = data[i + rOrder - 1];
            convertedData[i + 1] = data[i + gOrder - 1];
            convertedData[i + 2] = data[i + bOrder - 1];
            convertedData[i + 3] = data[i + aOrder - 1];
        }

        texture = ImGuiExt::Texture::fromBitmap(convertedData.data(), static_cast<int>(data.size()), static_cast<int>(width), static_cast<int>(height));
    }

    if (texture.isValid())
        ImGui::Image(texture, texture.getSize() * scale);

    if (ImGui::IsWindowHovered())
    {
        auto scrollDelta = ImGui::GetIO().MouseWheel;
        if (scrollDelta != 0.0F)
        {
            scale += scrollDelta * 0.1F;
            scale = std::clamp(scale, 0.1F, 10.0F);
        }
    }
}

// ─── HHA View ─────────────────────────────────────────────────────────────────

// Maps asset type enum to a readable name
static const char *assetTypeName(AssetTypeID id)
{
    switch (id)
    {
    case AssetTypeID::Asset_NONE:
        return "NONE";
    case AssetTypeID::Asset_Test_Bitmap:
        return "Test_Bitmap";
    case AssetTypeID::Asset_Shadow:
        return "Shadow";
    case AssetTypeID::Asset_Tree:
        return "Tree";
    case AssetTypeID::Asset_Sword:
        return "Sword";
    case AssetTypeID::Asset_Rock:
        return "Rock";
    case AssetTypeID::Asset_Grass:
        return "Grass";
    case AssetTypeID::Asset_Tuft:
        return "Tuft";
    case AssetTypeID::Asset_Stone:
        return "Stone";
    case AssetTypeID::Asset_Head:
        return "Head";
    case AssetTypeID::Asset_Cape:
        return "Cape";
    case AssetTypeID::Asset_Torso:
        return "Torso";
    case AssetTypeID::Asset_Font:
        return "Font";
    case AssetTypeID::Asset_FontGlyph:
        return "FontGlyph";
    case AssetTypeID::Asset_Bloop:
        return "Bloop";
    case AssetTypeID::Asset_Crack:
        return "Crack";
    case AssetTypeID::Asset_Drop:
        return "Drop";
    case AssetTypeID::Asset_Glide:
        return "Glide";
    case AssetTypeID::Asset_Music:
        return "Music";
    case AssetTypeID::Asset_Puhp:
        return "Puhp";
    case AssetTypeID::Asset_test_stereo:
        return "test_stereo";
    default:
        return "Unknown";
    }
}

class ViewHHA : public hex::View::Window
{
public:
    explicit ViewHHA() : hex::View::Window("HHA Explorer", "") {}

    void drawContent() override
    {
        auto *provider = hex::ImHexApi::Provider::get();
        if (provider == nullptr || !provider->isAvailable())
        {
            ImGui::TextDisabled("No file open.");
            return;
        }

        // Read and validate header
        HHAHeader header{};
        provider->read(0, &header, sizeof(header));

        if (header.magicValue != HHA_MAGIC_VALUE)
        {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Not a valid HHA file (bad magic).");
            ImGui::Text("Got: 0x%08X  Expected: 0x%08X", header.magicValue, HHA_MAGIC_VALUE);
            return;
        }

        // Header summary
        ImGui::Text("Version: %u  |  Asset Types: %u  |  Assets: %u  |  Tags: %u",
                    header.version, header.assetTypeCount, header.assetCount, header.tagCount);
        ImGui::Separator();

        // Read all asset types
        std::vector<HHAAssetType> assetTypes(header.assetTypeCount);
        provider->read(header.assetTypesOffset, assetTypes.data(),
                       header.assetTypeCount * sizeof(HHAAssetType));

        // Read all assets
        std::vector<HHAAsset> assets(header.assetCount);
        provider->read(header.assetsOffset, assets.data(),
                       header.assetCount * sizeof(HHAAsset));

        // Draw each asset type as a collapsible tree node
        for (const auto &assetType : assetTypes)
        {
            auto typeID = static_cast<AssetTypeID>(assetType.typeID);
            if (assetType.firstAssetIndex == assetType.onePastLastAssetIndex)
                continue;

            u32 count = assetType.onePastLastAssetIndex - assetType.firstAssetIndex;

            // ── Type override combo ──────────────────────────────
            AssetTypeID interpretAs = typeID;
            if (m_typeOverrides.contains(assetType.typeID))
                interpretAs = m_typeOverrides[assetType.typeID];

            auto label = std::string(assetTypeName(typeID)) + "  (" + std::to_string(count) + ")";

            ImGui::PushID(static_cast<int>(assetType.typeID));

            bool open = ImGui::TreeNode("##node", "%s", label.c_str());

            // Combo on same line as tree node
            ImGui::SameLine();
            ImGui::SetNextItemWidth(160);
            if (ImGui::BeginCombo("##interp", assetTypeName(interpretAs)))
            {
                for (u32 t = 0; t < static_cast<u32>(AssetTypeID::Asset_COUNT); t++)
                {
                    auto tid = static_cast<AssetTypeID>(t);
                    bool selected = (interpretAs == tid);
                    if (ImGui::Selectable(assetTypeName(tid), selected))
                        m_typeOverrides[assetType.typeID] = tid;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (open)
            {
                // Scrollable child for glyph-heavy types
                bool needsScroll = (count > 20);
                if (needsScroll)
                    ImGui::BeginChild("##scroll", ImVec2(0, 400), true);

                for (u32 ai = assetType.firstAssetIndex; ai < assetType.onePastLastAssetIndex; ai++)
                {
                    ImGui::PushID(static_cast<int>(ai));
                    drawAsset(provider, assets[ai], ai, interpretAs); // ← interpretAs, not typeID
                    ImGui::PopID();
                }

                if (needsScroll)
                    ImGui::EndChild();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    void drawHelpText() override
    {
        ImGui::Text("Opens and visualizes .hha asset pack files.");
        ImGui::Text("Bitmaps can be previewed inline. Scroll to zoom.");
    }

private:
    std::unordered_map<u32, AssetTypeID> m_typeOverrides;

    // Per-asset texture cache: assetIndex -> loaded texture + scale
    struct CachedBitmap
    {
        ImGuiExt::Texture texture;
        float scale = 1.0f;
    };
    std::unordered_map<u32, CachedBitmap> m_bitmapCache;

    void drawAsset(hex::prv::Provider *provider, const HHAAsset &asset,
                   u32 assetIndex, AssetTypeID typeID)
    {
        auto id = std::string("##asset") + std::to_string(assetIndex);

        if (isBitmapType(typeID) || isFontGlyphType(typeID))
        {
            u32 w = asset.data.bitmap.dim[0];
            u32 h = asset.data.bitmap.dim[1];

            bool open = ImGui::TreeNode("", "[%u]  %ux%u  align=(%.2f, %.2f)  offset=0x%llX",
                                        assetIndex, w, h,
                                        asset.data.bitmap.alignPercentage[0],
                                        asset.data.bitmap.alignPercentage[1],
                                        asset.dataOffset);

            if (open)
            {
                if (w > 0 && h > 0)
                {
                    // Load texture on demand
                    if (!m_bitmapCache.contains(assetIndex))
                    {
                        loadBitmapTexture(provider, asset.dataOffset, w, h, assetIndex);
                    }

                    auto &cached = m_bitmapCache[assetIndex];
                    if (cached.texture.isValid())
                    {
                        float maxWidth = ImGui::GetContentRegionAvail().x;
                        ImVec2 displaySize = cached.texture.getSize() * cached.scale;

                        // Auto-fit to panel width on first load
                        if (displaySize.x > maxWidth)
                            cached.scale = maxWidth / cached.texture.getSize().x;

                        displaySize = cached.texture.getSize() * cached.scale;
                        ImGui::Image(cached.texture, displaySize);

                        if (ImGui::IsItemHovered())
                        {
                            float delta = ImGui::GetIO().MouseWheel;
                            if (delta != 0.0f)
                            {
                                cached.scale += delta * 0.1f;
                                cached.scale = std::clamp(cached.scale, 0.1f, 10.0f);
                            }
                        }

                        ImGui::Text("Scale: %.1fx  (%ux%u px)",
                                    cached.scale,
                                    static_cast<u32>(cached.texture.getSize().x),
                                    static_cast<u32>(cached.texture.getSize().y));

                        ImGui::SameLine();
                        if (ImGui::SmallButton("Reset scale"))
                            cached.scale = 1.0f;
                    }
                }
                else
                {
                    ImGui::TextDisabled("(empty bitmap)");
                }
                ImGui::TreePop();
            }
        }
        else if (isSoundType(typeID))
        {
            u32 ch = asset.data.sound.channelCount;
            u32 smp = asset.data.sound.sampleCount;
            const char *chain = "";
            switch (asset.data.sound.chain)
            {
            case SoundChain::Loop:
                chain = " [LOOP]";
                break;
            case SoundChain::Advance:
                chain = " [ADVANCE]";
                break;
            default:
                break;
            }
            ImGui::Text("[%u]  channels=%u  samples=%u  (%.2fs)%s  offset=0x%llX",
                        assetIndex, ch, smp,
                        ch > 0 ? static_cast<float>(smp) / 48000.0f : 0.0f,
                        chain,
                        asset.dataOffset);
        }
        else if (isFontType(typeID))
        {
            u32 glyphCount = asset.data.font.glyphCount;
            if (ImGui::TreeNode("", "[%u]  glyphs=%u  ascender=%.1f  descender=%.1f  offset=0x%llX",
                                assetIndex, glyphCount,
                                asset.data.font.ascenderHeight,
                                asset.data.font.descenderHeight,
                                asset.dataOffset))
            {
                // Read and display glyph table
                std::vector<HHAFontGlyph> glyphs(glyphCount);
                provider->read(asset.dataOffset, glyphs.data(),
                               glyphCount * sizeof(HHAFontGlyph));

                if (ImGui::BeginTable("##glyphs", 3,
                                      ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_ScrollY,
                                      ImVec2(0, std::min<float>(200.0f, glyphCount * 20.0f))))
                {
                    ImGui::TableSetupColumn("Index");
                    ImGui::TableSetupColumn("Codepoint");
                    ImGui::TableSetupColumn("BitmapID");
                    ImGui::TableHeadersRow();

                    for (u32 g = 0; g < glyphCount; g++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u", g);
                        ImGui::TableSetColumnIndex(1);
                        u32 cp = glyphs[g].unicodeCodePoint;
                        if (cp >= 32 && cp < 127)
                            ImGui::Text("U+%04X  '%c'", cp, static_cast<char>(cp));
                        else
                            ImGui::Text("U+%04X", cp);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%u", glyphs[g].bitmapID);
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
        }
    }

    void loadBitmapTexture(hex::prv::Provider *provider,
                           u64 offset, u32 w, u32 h, u32 assetIndex)
    {
        size_t byteCount = static_cast<size_t>(w) * h * 4;
        std::vector<u8> raw(byteCount);
        provider->read(offset, raw.data(), byteCount);

        // HHA stores ARGB (0xAARRGGBB packed u32, written as BGRA bytes on LE)
        // Convert BGRA -> RGBA for ImGui
        std::vector<u8> rgba(byteCount);
        for (size_t i = 0; i < byteCount; i += 4)
        {
            rgba[i + 0] = raw[i + 2]; // R
            rgba[i + 1] = raw[i + 1]; // G
            rgba[i + 2] = raw[i + 0]; // B
            rgba[i + 3] = raw[i + 3]; // A
        }

        m_bitmapCache[assetIndex].texture = ImGuiExt::Texture::fromBitmap(
            rgba.data(), static_cast<int>(byteCount),
            static_cast<int>(w), static_cast<int>(h));
        m_bitmapCache[assetIndex].scale = 1.0f;
    }
};

// ─── Entry point ──────────────────────────────────────────────────────────────

IMHEX_PLUGIN_SETUP("HHA Plugin", "Manav", "HHA Explorer + custom bitmap visualizer")
{
    using ParamCount = pl::api::FunctionParameterCount;

    hex::ContentRegistry::PatternLanguage::addVisualizer(
        "custom_bitmap", drawCustomPackedBitmapVisualizer, ParamCount::exactly(4));

    hex::ContentRegistry::Views::add<ViewHHA>();
}