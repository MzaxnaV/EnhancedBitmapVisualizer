#include <hex/plugin.hpp>

#include <hex/api/content_registry.hpp>

#include <pl/patterns/pattern.hpp>

#include <imgui.h>

#include <hex/ui/imgui_imhex_extensions.h>

#include <vector>

void drawCustomPackedBitmapVisualizer(pl::ptrn::Pattern &, pl::ptrn::IIterable &, bool shouldReset, std::span<const pl::core::Token::Literal> arguments) {
    static ImGuiExt::Texture texture;
    static float scale = 1.0F;

    if (shouldReset) {
        auto pattern  = arguments[0].toPattern();
        auto width  = arguments[1].toUnsigned();
        auto height = arguments[2].toUnsigned();
        auto order = arguments[3].toUnsigned(); 
        auto data = pattern->getBytes(); 

        size_t size = data.size();
        
        std::vector<u8> convertedData(size);

        // NOTE: The order of RGBA channels in data, R -> 01, G -> 02, B -> 03, A -> 04,
        // for RGBA8 order will be 0x01020304.
        u8 rOrder = (order >> 24) & 0xFF;
        u8 gOrder = (order >> 16) & 0xFF;
        u8 bOrder = (order >> 8) & 0xFF;
        u8 aOrder = order & 0xFF;

        // TODO (optimization): vectorize this ?
        for (size_t i = 0; i < size; i += 4) {
            convertedData[i + 0] = data[i + rOrder - 1];
            convertedData[i + 1] = data[i + gOrder - 1];
            convertedData[i + 2] = data[i + bOrder - 1];
            convertedData[i + 3] = data[i + aOrder - 1];
        }

        texture = ImGuiExt::Texture(convertedData.data(), data.size(), ImGuiExt::Texture::Filter::Nearest, width, height);
    }

    if (texture.isValid())
        ImGui::Image(texture, texture.getSize() * scale);

    if (ImGui::IsWindowHovered()) {
        auto scrollDelta = ImGui::GetIO().MouseWheel;
        if (scrollDelta != 0.0F) {
            scale += scrollDelta * 0.1F;
            scale = std::clamp(scale, 0.1F, 10.0F);
        }
    }
}

IMHEX_PLUGIN_SETUP("Enhanced Bitmap Visualizer Plugin", "Manav", "Visualize bitmap with arbitary colour order") {
    using ParamCount = pl::api::FunctionParameterCount;
    hex::ContentRegistry::PatternLanguage::addVisualizer("custom_bitmap", drawCustomPackedBitmapVisualizer, ParamCount::exactly(4));
}