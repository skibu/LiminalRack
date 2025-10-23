#include <widget/OpenGlWidget.hpp>
#include <context.hpp>


namespace rack {
namespace widget {


void OpenGlWidget::step() {
	// Render every frame
	dirty = true;
	FramebufferWidget::step();
}


void OpenGlWidget::drawFramebuffer() {
	math::Vec fbSize = getFramebufferSize();
	glViewport(0.0, 0.0, fbSize.getX(), fbSize.getY());
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, fbSize.getX(), 0.0, fbSize.getY(), -1.0, 1.0);

	glBegin(GL_TRIANGLES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(fbSize.getX(), 0, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, fbSize.getY(), 0);
	glEnd();
}


} // namespace widget
} // namespace rack
