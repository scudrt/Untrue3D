/*
Soft Rasterization based on EGE and Eigen
Powered by SCUDRT
	* Geometry Stage except Orthogonal
	* Line Drawing
	* Triangle Clipping
	* Back Culling
	* Z-Buffer
	* Perspective-Correct Interpolation
	* Multi-Processing

	* Texture
	* .obj Model Loading
	* Optimized Rasterization

	* Texture
	* Static hard Light
	* Blinn-Phong shading
	* Colored Lighting
	* Plane Reflaction
TODO:
	* Optimize the codes
Advance:
	Shadow Anti-aliasing and anti-aliasing
	transparent
	reflaction base on cube map(sky box)
	particle system rendering
	gamma correction
	instanced rendering
	skinning
	post processing:
		bloom
		gaussian blur
	Ambient Occlusion
	screen space ambient occlusion
	fxaa
	deffered rendering

Requirement:
	FPS should be 50+(400 * 400)
*/

#include "UT3D.h"
#include "InputHandler.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace Eigen;
using namespace std;
using namespace chrono;
using namespace untrue;

//enable to show render info
#define UNTRUE_STAT

const float aspect = 16.0f / 9.0f;
const int WIN_WIDTH = 800;
const int WIN_HEIGHT = WIN_WIDTH / aspect;
const float FPS = 60.0f;
//delay_ms() is not precise, -1 is for performance
const float ELAPSE = 1000.0f / FPS;

untrue::UT3D* ut;

void loadFloor() {
	int base = ut->vertices.size();
	//floor
	ut->addVertex(ver(vec3(-4000, -4000, 0), vec3(0.8, 0.8, 0.8)));
	ut->addVertex(ver(vec3(4000, -4000, 0), vec3(0.8, 0.8, 0.8)));
	ut->addVertex(ver(vec3(-4000, 4000, 0), vec3(0.8, 0.8, 0.8)));
	ut->addVertex(ver(vec3(4000, 4000, 0), vec3(0.8, 0.8, 0.8)));

	for (int i = 0;i < 4;++i) {
		ut->vertices[base + i].receiveShadow = true;
		ut->vertices[base + i].normal = vec3(0.0f, 0.0f, 1.0f);
	}
	ut->addTriangle(base + 0, base + 1, base + 2);
	ut->addTriangle(base + 1, base + 3, base + 2);
}

void setReflactionPlane() {
	int base = ut->vertices.size();
	int refid = ut->setReflaction(vec3(500, 0, 100), vec3(-1, 0, 0));
	ut->addVertex(ver(vec3(500, 400, 0), vec2(-1, -1)));
	ut->addVertex(ver(vec3(500, -400, 0), vec2(-1, -1)));
	ut->addVertex(ver(vec3(500, 400, 800), vec2(-1, -1)));
	ut->addVertex(ver(vec3(500, -400, 800), vec2(-1, -1)));

	for (int i = 0;i < 4;++i) {
		ut->vertices[base + i].texIndex = refid;
		ut->vertices[base + i].normal = vec3(-1, 0, 0);
	}

	ut->addTriangle(base, base + 1, base + 3);
	ut->addTriangle(base, base + 3, base + 2);
}

void loadBoxes() {
	ut->addTexture("./texture1.bmp");
	ut->addTexture("./texture2.bmp");
	ut->addTexture("./texture3.bmp");

	for (int i = 0;i < 3;++i) {
		for (int j = 0;j < 3;++j) {
			for (int k = 0;k < 3;++k) {
				ver vs[24] = {
					//up
					ver(vec3(-400 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(0, 0)),
					ver(vec3(-200 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(1, 0)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(1, 1)),
					ver(vec3(-400 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(0, 1)),
					//down
					ver(vec3(-400 + i * 300, 100 + j * 300, 300 * k), vec2(0, 0)),
					ver(vec3(-400 + i * 300, 300 + j * 300, 300 * k), vec2(1, 0)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 300 * k), vec2(1, 1)),
					ver(vec3(-200 + i * 300, 100 + j * 300, 300 * k), vec2(0, 1)),
					//left
					ver(vec3(-400 + i * 300, 100 + j * 300, 300 * k), vec2(0, 0)),
					ver(vec3(-400 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(1, 0)),
					ver(vec3(-400 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(1, 1)),
					ver(vec3(-400 + i * 300, 300 + j * 300, 300 * k), vec2(0, 1)),
					//right
					ver(vec3(-200 + i * 300, 100 + j * 300, 300 * k), vec2(0, 0)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 300 * k), vec2(1, 0)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(1, 1)),
					ver(vec3(-200 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(0, 1)),
					//front
					ver(vec3(-400 + i * 300, 100 + j * 300, 300 * k), vec2(0, 0)),
					ver(vec3(-200 + i * 300, 100 + j * 300, 300 * k), vec2(1, 0)),
					ver(vec3(-200 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(1, 1)),
					ver(vec3(-400 + i * 300, 100 + j * 300, 200 + 300 * k), vec2(0, 1)),
					//back
					ver(vec3(-400 + i * 300, 300 + j * 300, 300 * k), vec2(0, 0)),
					ver(vec3(-400 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(1, 0)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 200 + 300 * k), vec2(1, 1)),
					ver(vec3(-200 + i * 300, 300 + j * 300, 300 * k), vec2(0, 1)),
				};

				int base = ut->vertices.size();

				//set normals for lighting test
				vec3 normals[6] = {
					vec3(0, 0, 1), vec3(0, 0, -1), //up down
					vec3(-1, 0, 0), vec3(1, 0, 0), //left right
					vec3(0, -1, 0), vec3(0, 1, 0) //front back
				};
				for (int norm = 0;norm < 6;++norm) {
					for (int index = norm * 4;index < (norm + 1) * 4;++index) {
						vs[index].normal = normals[norm];
						vs[index].receiveShadow = true;
						vs[index].texIndex = ut->textureBuffer.size() - 3 + i;
						ut->addVertex(vs[index]);
					}
				}

				for (int v = 0;v < 24;v += 4) {
					ut->addTriangle(base + v, base + v + 1, base + v + 2);
					ut->addTriangle(base + v, base + v + 2, base + v + 3);
				}
			}
		}
	}
}

void start() {
	ut = untrue::UT3D::instance();

	ut->init(WIN_WIDTH, WIN_HEIGHT, NORMAL);
	ut->setStatEnable(true);

	loadFloor();
	setReflactionPlane();
	loadBoxes();

	//ut->loadModel("./skull/", "skull.obj");
	//ut->loadModel("./Cats_obj/", "Cats_obj.obj");

	//Config Lightings
	Light* light = new PointLight(true, 2048);
	light->setPosition(vec3(20, -1200, 1200));
	light->setIntensity(80.0f);
	ut->addLighting(light);

	light = new SpotLight(true, 2048);
	light->setPosition(vec3(-1000, 50, 800));
	light->rotateBy(vec3(-70, 0, -90));
	light->setIntensity(50.0f);
	ut->addLighting(light);

	//Config camera
	ut->setPerspective(70.0f);
	ut->setCameraPosition(vec3(0, -700, 500));
	ut->setCameraLookat(vec3(0, 1, 0));
	ut->setCameraUp(vec3(0, 0, 1));

	//ut->setReflaction(vec3(0, 0, 10), vec3(0.4472, 0, 0.8944));
}

void paint(float deltaTime) {
	deltaTime /= 1000.0f; //ms -> s
	static float t = 0.0f;
	t += deltaTime;

	//处理3D漫游
	InputHandler::handle(deltaTime);

	ut->draw(deltaTime);
}

int main() {
	auto timeAtFrameStart = steady_clock::now(),
		timeAtFrameEnd = timeAtFrameStart;
	float delta, freeTime, computeTime = 0.0f, curDelta = 0.0f,
		sumOfDelayTime = 0.0f, curFPS = 0.0f;
	int fpsTick = 0;
	const Stat* stat;

	start();
	while (!InputHandler::needExit) {
		//时间记录
		timeAtFrameEnd = steady_clock::now();
		//ns to ms
		delta = (timeAtFrameEnd - timeAtFrameStart).count() / 1000000.0f;
		timeAtFrameStart = steady_clock::now();

		sumOfDelayTime += delta;
		++fpsTick;

		//drawing
		ut->clearDevice();
		paint(delta);

		computeTime = (steady_clock::now() - timeAtFrameStart).count() / 1000000.0f;
		freeTime = max(0.0f, ELAPSE - computeTime);

		//update FPS every half second
		if (sumOfDelayTime >= 500.0f) {
			curFPS = fpsTick * 1000.0f / sumOfDelayTime;
			curDelta = sumOfDelayTime / fpsTick;
			fpsTick = 0;
			sumOfDelayTime = 0.0f;
		}
#ifdef UNTRUE_STAT
		xyprintf(10, 10, "FPS: %.1f  delta: %.2f ms", curFPS, curDelta);
		stat = &ut->getRenderStat();
		xyprintf(10, 30, "geometry: %.2lf ms", stat->geometryTime);
		xyprintf(10, 50, "rasterization: %.2lf ms", stat->rasterizationTime);
		xyprintf(WIN_WIDTH - 150, 10, "vertices: %d", ut->vertices.size());
		xyprintf(WIN_WIDTH - 150, 30, "faces: %d", stat->renderingFaces);
#endif
		
		delay_ms(freeTime);
	}

	return 0;
}
