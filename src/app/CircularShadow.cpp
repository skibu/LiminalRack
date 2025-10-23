#include <app/CircularShadow.hpp>


namespace rack {
namespace app {


CircularShadow::CircularShadow() {
	blurRadius = 0;
	opacity = 0.15;
}

void CircularShadow::draw(const DrawArgs& args) {
    if (opacity <= 0.0) return;

    math::Vec center = getSize().div(2.0);
    float radius = center.getX();
    NVGcolor icol = nvgRGBAf(0.0, 0.0, 0.0, opacity);
    NVGcolor ocol = nvgRGBAf(0.0, 0.0, 0.0, 0.0);
    nvgBeginPath(args.vg);
    if (blurRadius > 0.0) {
        nvgRect(args.vg, -blurRadius, -blurRadius, getWidth() + 2 * blurRadius,
                getHeight() + 2 * blurRadius);
        NVGpaint paint =
            nvgRadialGradient(args.vg, center.getX(), center.getY(),
                              radius - blurRadius, radius, icol, ocol);
        nvgFillPaint(args.vg, paint);
    } else {
        nvgCircle(args.vg, center.getX(), center.getY(), radius);
        nvgFillColor(args.vg, icol);
    }
    nvgFill(args.vg);
}

} // namespace app
} // namespace rack
