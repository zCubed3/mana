#include "mana_pipeline.hpp"

#include <mana/builders/mana_render_pass_builder.hpp>

using namespace ManaVK;

void ManaPipeline::set_render_pass_builder(std::unique_ptr<Builders::ManaRenderPassBuilder> builder) {
    render_pass_builder = std::move(builder);
}

std::shared_ptr<ManaRenderPass> ManaPipeline::build_render_pass(ManaInstance *owner) {
    return render_pass_builder->build(owner);
}
