#ifndef MANA_MANA_PIPELINE_HPP
#define MANA_MANA_PIPELINE_HPP

#include <memory>

namespace ManaVK::Builders {
    class ManaRenderPassBuilder;
}

namespace ManaVK {
    class ManaInstance;
    class ManaRenderPass;

    class ManaPipeline {
    protected:
        std::unique_ptr<Builders::ManaRenderPassBuilder> render_pass_builder;

    public:
        //
        // Methods
        //
        std::shared_ptr<ManaRenderPass> build_render_pass(ManaInstance *owner);

        //
        // Setters
        //
        void set_render_pass_builder(std::unique_ptr<Builders::ManaRenderPassBuilder> builder);
    };
}

#endif//MANA_MANA_PIPELINE_HPP
