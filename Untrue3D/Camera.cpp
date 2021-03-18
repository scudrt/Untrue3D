#include <iostream>
#include <windows.h>
#include <chrono>
#include <ctime>

#include "UT3D.h"
#include "Camera.h"
#include "graphics.h"

using namespace untrue;
using namespace std;
using namespace std::chrono;

Camera::Camera() {
	depthBuffer = nullptr;
	frags = nullptr;

	__rasthreads = nullptr;
	__framethreads = nullptr;

	isPerspective = false;
	isBackCulling = true;
	reflactionEnabled = false;

	rotation.setZero();
	projection.setIdentity();
	direction = vec3(0, 0, 1);
}

Camera::~Camera() {
	if (frags) {
		delete[] depthBuffer[0];
		delete[] depthBuffer;

		delete[] colorBuffer[0];
		delete[] colorBuffer;

		delete[] frags[0];
		delete[] frags;
	}
	__killThreads();
}

//Must call once before render()
void Camera::setCamera(int width, int height, RenderMode renderMode) {
	this->screenWidth = width;
	this->screenHeight = height;
	this->renderMode = renderMode;

	if (renderMode == NORMAL) {
		colorBuffer = new int*[screenHeight];
		colorBuffer[0] = new int[screenHeight * screenWidth];
		for (int i = 1;i < screenHeight;++i) {
			colorBuffer[i] = colorBuffer[i - 1] + screenWidth;
		}
	}

	screenMapping << width / 2.0, 0, 0, width / 2.0,
		0, height / 2.0, 0, height / 2.0,
		0, 0, 1, 0,
		0, 0, 0, 1;

	if (frags) {
		delete[] depthBuffer[0];
		delete[] depthBuffer;
		delete[] frags[0];
		delete[] frags;
	}

	depthBuffer = new float*[height];
	frags = new ver*[height];

	depthBuffer[0] = new float[height * width];
	frags[0] = new ver[height * width];

	for (int i = 1;i < height;++i) {
		depthBuffer[i] = depthBuffer[i - 1] + width;
		frags[i] = frags[i - 1] + width;
	}
}

//set it false to optimize shadowmap
void Camera::setBackCulling(bool backCulling) {
	isBackCulling = backCulling;
}

//setting it true will enable statistics while rendering
void Camera::setStatEnable(bool enable) {
	isStatEnable = enable;
}

const Stat& Camera::getRenderStat() {
	return renderStat;
}

const RenderMode& Camera::getRenderMode() {
	return renderMode;
}

void Camera::bindVertices(std::vector<ver, Eigen::aligned_allocator<ver> >* sceneVs) {
	this->vs = sceneVs;
}

void Camera::bindTriangles(std::vector<tri>* sceneTs) {
	this->ts = sceneTs;
}

void Camera::normalWorldToCamera(vec3& normal) {
	normal = viewTransform.block(0, 0, 3, 3) * normal;
}

//updateCameraTransform() must be called once before tthese two functions below
void Camera::worldToScreen(vec4& v) {
	v = _worldToCVV * v;
	float z = v(3);
	if (isPerspective) v /= z;
	v(3) = 1.0f;
	v = screenMapping * v;
	v(3) = z;
}

void Camera::screenToWorld(vec4& v) {
	float z = v(3);
	v(0) = v(0) / screenWidth * 2.0f - 1.0f;
	v(1) = v(1) / screenHeight * 2.0f - 1.0f;
	if (isPerspective) {
		v(0) *= z;
		v(1) *= z;
		v(2) = (z * n + z * f - 2.0f * n * f) / (f - n);
	}
	v = _CVVToWorld * v;
}

void Camera::setReflaction(const mat4& reflaction) {
	this->reflactionEnabled = true;
	this->isBackCulling = !this->isBackCulling;
	this->reflaction = reflaction;
}

void Camera::reflactionEnable(bool enable) {
	if (enable != this->reflactionEnabled) {
		this->isBackCulling = !this->isBackCulling;
	}
	this->reflactionEnabled = enable;
}

void Camera::setPerspective(float fov, float n, float f) {
	this->isPerspective = true;

	this->fov = fov;
	this->n = n;
	this->f = f;
	float asp = 1.0f * screenWidth / screenHeight;
	float tant = tan(fov / 360.0f * PI);

	//normalize to range [-1.0, 1.0]
	//depth is [0, 1]
	projection << 1.0f / (asp * tant), 0.0, 0.0, 0.0,
		0.0, 1.0f / tant, 0.0, 0.0,
		//0.0, 0.0, f / (f - n), -n * f / (f - n), //[0, 1]
		0.0, 0.0, (n + f) / (f - n), -2.0f * n * f / (f - n), //[-1, 1]
		0.0, 0.0, 1.0f, 0.0;
}

void Camera::setOrthogonal(float depth) {
	this->isPerspective = false;
	n = 0; f = depth;
	projection << 2.0f / screenWidth, 0, 0, 0, //[-1, 1]
		0, 2.0f / screenHeight, 0, 0, //[-1, 1]
		0, 0, 2.0f / depth, -1.0f, //[-1, 1]
		0, 0, 1.0f, 0;
}

float* Camera::getDepthBuffer() {
	return this->depthBuffer[0];
}

//retrun screen space vertex array after early-z
ver* Camera::getFragBuffer() {
	return this->frags[0];
}

const int* Camera::getColorBuffer() {
	return this->colorBuffer[0];
}

int Camera::getScreenWidth() {
	return this->screenWidth;
}

int Camera::getScreenHeight() {
	return this->screenHeight;
}

//return rotation matrix base on Camera::rotation
mat3 Camera::getRotation() {
	const float k = -ege::PI / 180.0f; //取反方向并转为弧度
	float c[3], s[3]; //cosine and sine
	for (int i = 0;i < 3;++i) {
		c[i] = std::cos(k * rotation(i));
		s[i] = std::sin(k * rotation(i));
	}
	mat3 ret;
	ret << c[1] * c[2] - s[0] * s[1] * s[2], -c[0] * s[2], s[1] * c[2] + s[0] * c[1] * s[2],
		c[1] * s[2] + s[0] * s[1] * c[2], c[0] * c[2], s[1] * s[2] - s[0] * c[1] * c[2],
		-c[0] * s[1], s[0], c[0] * c[1];
	return ret;
}

//cheap camera translation
void Camera::translateBy(const vec3& offset) {
	this->position += this->getRotation() * offset;
}
//cheap camera rotation
void Camera::rotateBy(const vec3& degree) {
	this->rotation -= degree;
}

//Must call before render()
void Camera::setPosition(const vec3& position) {
	this->position = position;
}
void Camera::setLookat(const vec3& lookat) {
	this->lookat = lookat;
}
void Camera::setUp(const vec3& up) {
	this->up = up;
}

const vec3& Camera::getPosition() {
	return this->position;
}

const vec3& Camera::getDirection() {
	return this->direction;
}

//Recalculate camera transformation matrix by new position, lookat and up
void Camera::updateCameraState() {
	mat3 rotator = this->getRotation();
	vec3 u,
		v = rotator * this->up,
		w = rotator * this->lookat;

	this->direction = rotator * vec3(0, 1, 0);

	w.normalize();

	u = w.cross(v);
	u.normalize();

	v = u.cross(w);
	v.normalize();

	viewTransform << u.transpose(), u.transpose() * -position,
		v.transpose(), v.transpose() * -position,
		w.transpose(), w.transpose() * -position,
		0, 0, 0, 1;

	if (this->reflactionEnabled) {
		viewTransform = viewTransform * reflaction;
	}

	_worldToCVV = projection * viewTransform;
	_CVVToWorld = _worldToCVV.inverse();
}

int Camera::_getClipCode(const vec4& p) {
	int code = 0, weight = 1;
	float z = isPerspective ? p(3) : 1.0f;
	for (int i = 0;i < 3;++i) {
		if (p(i) < -z) {
			code |= weight;
		} else if (p(i) > z) {
			code |= (weight << 1);
		}
		weight <<= 2;
	}
	return code;
}

//Make line ab clip at near plane
void Camera::nearClip(ver& a, ver& b) {
	ver delta = b - a;
	a += delta * (this->n - a.position(3)) / delta.position(3);
}

//Solve near-clip triangles and store new triangles into tBuffer
//No support for multithreading
void Camera::triangleClip(tri ta) {
	int codes[3], code = 0;
	for (int i = 0;i < 3;++i) {
		codes[i] = _getClipCode(vBuffer[ta(i)].position);
		code ^= codes[i];
	}
	if (codes[0] & codes[1] & codes[2]) return; //all out of CVV
	if ((codes[0] & 16) || (codes[1] & 16) || (codes[2]) & 16) {
		//one or two vertices clip at near plane
		int left = code & 16 ? 16 : 0, right, k, size = vBuffer.size();
		//get the special vertex index
		for (k = 0;k <= 2;++k) {
			if ((codes[k] & 16) == left) {
				break;
			}
		}
		left = (k - 1 + 6) % 3; //compute near by index
		right = (k + 1 + 6) % 3;
		ver v(vBuffer[ta(k)]), va(vBuffer[ta(left)]), vb(vBuffer[ta(right)]);
		if (code & 16) { //one vertex clips
			nearClip(v, va);
			vBuffer.emplace_back(v); //size
			v = vBuffer[ta(k)]; //change v first!!!
			ta(k) = size;
			tBuffer.emplace_back(ta);
			//another triangle
			nearClip(v, vb);
			vBuffer.emplace_back(v); //size + 1
			ta = tri(size, size + 1, ta(right));
			tBuffer.emplace_back(ta);
		} else { //two vertices clips
			nearClip(va, v);
			nearClip(vb, v);
			vBuffer.emplace_back(va); //size
			vBuffer.emplace_back(vb); //size + 1
			ta(left) = size;
			ta(right) = size + 1;
			tBuffer.emplace_back(ta);
		}
	} else {
		tBuffer.emplace_back(ta);
	}
}

//it's called when first run render()
void Camera::__initThreads() {
	depthLock = new mutex[screenHeight];

	rasReady = new atomic_bool[RASTHREAD_SIZE];
	__rasthreads = new thread[RASTHREAD_SIZE];
	__rasthreadState = new volatile ThreadState[RASTHREAD_SIZE];
	for (int i = 0;i < RASTHREAD_SIZE;++i) {
		rasReady[i].store(false);
		__rasthreadState[i] = RUNNING;
		__rasthreads[i] = std::thread(&Camera::rasterizationThread, this, i);
	}
	if (this->renderMode == NORMAL) {
		frameReady = new atomic_bool[FRAMETHREAD_SIZE];
		__framethreads = new thread[FRAMETHREAD_SIZE];
		__framethreadState = new volatile ThreadState[FRAMETHREAD_SIZE];
		for (int i = 0;i < FRAMETHREAD_SIZE;++i) {
			frameReady[i].store(false);
			__framethreadState[i] = RUNNING;
			__framethreads[i] = std::thread(&Camera::frameThread, this, i);
		}
	}
}

void Camera::__resumeAllThreads(const char* type) {
	if (type == "rasterize") {
		renderingFace.store(0); //statistics data
		rasFinished.store(0);
		for (int i = 0;i < RASTHREAD_SIZE;++i) {
			rasReady[i].store(true);
		}
		threadRasCon.notify_one();
	} else if (type == "frame") {
		frameFinished.store(0);
		for (int i = 0;i < FRAMETHREAD_SIZE;++i) {
			frameReady[i].store(true);
		}
		threadFrameCon.notify_one();
	}
}

//wait until all rasterizing threads finish their works
void Camera::__waitForAllThreads(const char* type) {
	if (type == "rasterize") {
		unique_lock<mutex> locker(rasmutex);
		while (rasFinished != RASTHREAD_SIZE) mainRasCon.wait_for(locker, chrono::milliseconds(30));
		locker.unlock();

		//record statistical data
		renderStat.renderingFaces = renderingFace.load();
	} else if (type == "frame") {
		unique_lock<mutex> locker(framemutex);
		while (frameFinished != FRAMETHREAD_SIZE) mainFrameCon.wait_for(locker, chrono::milliseconds(30));
		locker.unlock();
	}
}


void Camera::__killThreads() {
	if (__rasthreads) {
		for (int i = 0;i < RASTHREAD_SIZE;++i) {
			__rasthreadState[i] = EXIT;
		}
		__resumeAllThreads("rasterize");
		for (int i = 0;i < RASTHREAD_SIZE;++i) {
			__rasthreads[i].join();
		}
		delete[] rasReady;
		delete[] depthLock;
		__rasthreads = nullptr;
	}
	if (__framethreads) {
		for (int i = 0;i < FRAMETHREAD_SIZE;++i) {
			__framethreadState[i] = EXIT;
		}
		__resumeAllThreads("frame");
		for (int i = 0;i < FRAMETHREAD_SIZE;++i) {
			__framethreads[i].join();
		}
		delete[] frameReady;
		__framethreads = nullptr;
	}
}

//return true if triangle is backward, for back culling algorithm
bool Camera::isBackward(const tri& t) {
	vec3 a = (vBuffer[t(1)].position - vBuffer[t(0)].position).head(3),
		b = (vBuffer[t(2)].position - vBuffer[t(1)].position).head(3);
	a(2) = b(2) = 0.0;
	return a.cross(b)(2) <= 0.0;
}

bool Camera::isBackward(const ver& a, const ver& b, const ver& c) {
	vec3 va = (b.position - a.position).head(3),
		vb = (c.position - b.position).head(3);
	va(2) = vb(2) = 0.0;
	return va.cross(vb)(2) <= 0.0;
}

static inline float _cross(const vec2& a, const vec2& b) {
	return a(0) * b(1) - a(1) * b(0);
}
static inline int getColorValue(const vec3& c) {
	return int(c(0)) << 16
		| int(c(1)) << 8
		| int(c(2));
}

//multi-thread rasterizing algorithm
//TODO: little line gaps when the triangle is flat in screen space
void Camera::rasterizeTriangle(const tri& t) {
	ver a(vBuffer[t(0)]), b(vBuffer[t(1)]), c(vBuffer[t(2)]);
	//make rasterizing compatible with front culling
	if (isBackCulling == false) swap(b, c);
	int left = min(a.position(0), min(b.position(0), c.position(0))),
		right = max(a.position(0), max(b.position(0), c.position(0))),
		top = max(a.position(1), max(b.position(1), c.position(1))),
		down = ceil(min(a.position(1), min(b.position(1), c.position(1))));
	//2D clipping
	left = max(0, left); right = min(screenWidth - 1, right);
	top = min(screenHeight - 1, top); down = max(0, down);
	vec2 d[3] = { (b.position - a.position).head(2),
					(c.position - b.position).head(2),
					(a.position - c.position).head(2) };
	ver v(vec3(left, down, 0)), vBase, vUp, vRight;
	long long l, r; //prevents calculation overflow error
	float f[3], temp = -_cross(d[0], d[2]), pz; //2x area of triangle
	if ((temp <= 0)) return;
	//pre-computing
	f[0] = _cross(d[0], (v.position - a.position).head(2));
	f[1] = _cross(d[1], (v.position - b.position).head(2));
	f[2] = _cross(d[2], (v.position - c.position).head(2));
	// Perspective-Correct Interpolation
	if (isPerspective) {
		pz = a.position(3); a *= pz; a.position(3) = pz;
		pz = b.position(3); b *= pz; b.position(3) = pz;
		pz = c.position(3); c *= pz; c.position(3) = pz;
	}
	vBase = (c * f[0] + a * f[1] + b * f[2]) / temp;
	vUp = (c * d[0](0) + a * d[1](0) + b * d[2](0)) / temp;
	vRight = (c * d[0](1) + a * d[1](1) + b * d[2](1)) / -temp;
	float *depthptr;
	ver* fragptr;
	//Rasterizating
	for (int y = down;y <= top;++y) {
		l = 0; r = right - left;

		for (int i = 0;i < 3;++i) {
			temp = d[i](1) == 0 ? -screenWidth * (f[i] >= 0) : f[i] / d[i](1);
			f[i] += d[i](0);
			-d[i](1) < 0 ? r = min(r, temp) : l = max(l, ceil(temp));
		}

		if (l <= r) {
			v = vBase + vRight * l;
			l += left; r += left;
			depthptr = &depthBuffer[y][l];
			fragptr = &frags[y][l];

			for (depthLock[y].lock();l <= r;++l) { //start rasterizing and do early-z
				pz = v.position(3);
				if (isPerspective) pz = 1.0f / pz;
				if (pz < *depthptr) {
					(*depthptr) = pz;
					if (this->renderMode != DEPTH) {
						if (!isPerspective) { (*fragptr) = v; }
						else { (*fragptr) = v * pz; }
					}
				}
				v += vRight;
				++depthptr;
				++fragptr;
			}
			depthLock[y].unlock();
		}
		vBase += vUp;
	}
}

//clip code: y -y x -x
int Camera::clipcode2d(const vec2& v) {
	int ret = 0;
	if (v(0) < 0.0f) ret |= 1;
	if (v(0) >= screenWidth) ret |= 2;
	if (v(1) < 0.0f) ret |= 4;
	if (v(1) >= screenHeight) ret |= 8;
	return ret;
}

//return whether the line is retained
bool Camera::lineClip(vec2& a, vec2& b) {
	int codea = clipcode2d(a), codeb = clipcode2d(b),
		code = codea | codeb;
	//all out of screen
	if (codea & codeb) return false;
	//all in the screen
	if (!code) return true;
	//to be done
	;
	return true;
}

//no 2D clipping, so it might costs while drawing out of screen
void Camera::drawLine(const ver& a, const ver& b) {
	vec2 pa(a.position.head(2)), pb(b.position.head(2));
	lineClip(pa, pb);
	float dx = pb(0) - pa(0),
		dy = pb(1) - pa(1),
		x = pa(0), y = pa(1), k;
	k = max(abs(dx), abs(dy)); //line slope
	dx /= k; dy /= k;

	int tick = int(k); //count of line pixels in screen space
	int ix, iy;
	for (int i = 0;i <= tick + 1;++i) { //draw tick+1 times
		ix = int(x), iy = int(y);
		if ((ix | iy) >= 0 && ix < screenWidth && iy < screenHeight) {
			frags[iy][ix].color << 200, 200, 200;
		}
		x += dx; y += dy;
	}
}

void Camera::wireframeTriangle(const tri& t) {
	drawLine(vBuffer[t(0)], vBuffer[t(1)]);
	drawLine(vBuffer[t(1)], vBuffer[t(2)]);
	drawLine(vBuffer[t(0)], vBuffer[t(2)]);
}

void Camera::rasterizationThread(int tid) {
	int size;
	while (__rasthreadState[tid] != EXIT) {
		unique_lock<mutex> locker(rasmutex);
		while (!rasReady[tid].load()) {
			threadRasCon.wait_for(locker, chrono::milliseconds(30));
		}
		//threadCon.wait(locker, [this, tid]() {return rasReady[tid].load();});
		rasReady[tid].store(false);
		
		threadRasCon.notify_one();
		locker.unlock();

		size = tBuffer.size();
		for (int i = tid;i < size;i += RASTHREAD_SIZE) {
			if (isBackCulling xor isBackward(tBuffer[i])) {
				++renderingFace;
				if (renderMode == NORMAL || renderMode == DEPTH) {
					rasterizeTriangle(tBuffer[i]);
				} else if (renderMode == WIREFRAME) {
					wireframeTriangle(tBuffer[i]);
				} else {
					//error
				}
			}
		}

		++rasFinished;
		if (rasFinished.load() == RASTHREAD_SIZE) {
			mainRasCon.notify_one();
		}
	}
}

//deal with frame functions
void Camera::frameThread(int tid) {
	int* cptr; //color buffer pointer
	float* dptr; //depth buffer pointer
	ver* fptr; //fragment buffer pointer
	vec4 temp, norm(0, 0, 0, 0);
	vec3 lightColor, fragColor; //vector formed color
	int bufferStep = screenWidth * (FRAMETHREAD_SIZE - 1), lightingSize;
	float shadow, tempshadow;
	UT3D* ut = UT3D::instance();
	while (__framethreadState[tid] != EXIT) {
		//wait for main thread to wake up
		unique_lock<mutex> locker(framemutex);
		while (!frameReady[tid].load()) {
			threadFrameCon.wait_for(locker, chrono::milliseconds(30));
		}
		frameReady[tid].store(false);
		//nofity other lighting threads
		threadFrameCon.notify_one();
		locker.unlock();

		lightingSize = ut->lightings.size();
		cptr = this->colorBuffer[0] + tid * screenWidth;
		dptr = depthBuffer[0] + tid * screenWidth;
		fptr = frags[0] + tid * screenWidth;
		for (int y = tid;y < screenHeight;y += FRAMETHREAD_SIZE) {
			for (int x = 0;x < screenWidth;++x, ++cptr, ++dptr, ++fptr) {
				if (renderMode == NORMAL) {
					if (*dptr >= 0x505050) continue; //not out of max view depth
					if (fptr->texIndex != -1) { //texid != -1 means texture enabled
						if (fptr->uv(0) < 0) { //reflaction texture
							fragColor = Color::toColorVector(
								ut->textureBuffer[fptr->texIndex].getColor(
									1.0f * x / screenWidth,
									1.0f * y / screenHeight
								)
							);
						} else {
							fragColor = Color::toColorVector(
								ut->textureBuffer[fptr->texIndex].getColor(fptr->uv)
							);
						}
					} else {
						fragColor = fptr->color;
					}
					lightColor = vec3(0, 0, 0);

					//transform the fragment from screen space to world space
					fptr->position << x, y, 1.0f, (*dptr);
					screenToWorld(fptr->position);

					//lighting computation
					float intensity, totalInten = 0.0f;
					for (auto it = ut->lightings.begin();it != ut->lightings.end();++it) {
						intensity = (*it)->light(
							*fptr,
							this->getPosition() - fptr->position.head(3)
						);
						lightColor += intensity * (*it)->getColor();
						totalInten += intensity;
					}
					fragColor *= totalInten;
					Color::mul(fragColor, lightColor);
				} else {
					fragColor = fptr->color;
				}
				*cptr = Color::toRGBValue(fragColor);
			}
			cptr += bufferStep;
			dptr += bufferStep;
			fptr += bufferStep;
		}

		//add up the count of finished threads
		++frameFinished;
		if (frameFinished.load() == FRAMETHREAD_SIZE) { //time to end the lighting work
			mainFrameCon.notify_one();
		}
	}
}

void Camera::render() {
	auto t_start = steady_clock::now();

	//update transform matrix after user's inputs every frame
	updateCameraState();

	memset(depthBuffer[0], 0x50, //0x50505050 is a large number for float
		sizeof(float) * this->screenWidth * this->screenHeight);
	memset(frags[0], 0,
		sizeof(ver) * this->screenWidth * this->screenHeight);

	/* Geometry Stage */
	//convert world space to CVV space
	int size = vs->size();
	for (int i = 0;i < size;++i) {
		vBuffer.emplace_back((*vs)[i]);
		vBuffer[i].position = _worldToCVV * vBuffer[i].position;
	}

	for (std::vector<tri>::const_iterator it = ts->begin();
		it != ts->end();++it) {
		triangleClip(*it);
	}

	float pz;
	for (std::vector<ver>::iterator it = vBuffer.begin();
		it < vBuffer.end();++it) {
		pz = it->position(3);
		if (isPerspective) {
			if (pz > 0) {
				it->position /= pz;
				it->position = screenMapping * it->position;
				it->position(3) = 1.0f / pz; //perspective interpolation
			}
		} else {
			it->position(3) = 1.0f;
			it->position = screenMapping * it->position;
			it->position(3) = pz;
		}
	}

	if (this->isStatEnable) {
		renderStat.geometryTime = (steady_clock::now() - t_start).count() / 1000000.0f;
		t_start = steady_clock::now();
	}

	if (!__rasthreads) {
		__initThreads();
	}
	//Raterization Stage, multi-threading
	__resumeAllThreads("rasterize");
	__waitForAllThreads("rasterize");

	if (this->renderMode == NORMAL) { //light camera only render depth map
		memset(colorBuffer[0], 0, sizeof(int) * screenWidth * screenHeight);
		__resumeAllThreads("frame");
		__waitForAllThreads("frame");
	}

	if (this->isStatEnable) {
		renderStat.rasterizationTime = (steady_clock::now() - t_start).count() / 1000000.0f;
	}

	vBuffer.clear();
	tBuffer.clear();
}
