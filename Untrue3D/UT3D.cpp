#include "UT3D.h"
#include "Eigen/Geometry" //for vector cross calculation

#include <iostream>
#include <fstream>
#include <chrono>
#include <map>

using namespace Eigen;
using namespace std;
using namespace untrue;

UT3D* UT3D::__inst;

UT3D* UT3D::instance() {
	if (__inst == nullptr) {
		__inst = new UT3D();
	}
	return __inst;
}

UT3D::UT3D() {
	;
}

UT3D::~UT3D() {
	onFinish();
}

void UT3D::init(int width, int height, RenderMode renderMode) {
	WIN_HEIGHT = height;
	WIN_WIDTH = width;

	this->reflactionTextureIndex = -1;

	//graphic configurations
	initgraph(width, height);
	ege::setrendermode(RENDER_MANUAL);
	ege::setbkcolor(0);
	ege::setcolor(0xffffff);

	this->mainCamera = new Camera();
	mainCamera->bindVertices(&vertices);
	mainCamera->bindTriangles(&triangles);
	mainCamera->setCamera(width, height, renderMode);
}

void UT3D::onFinish() {
	delete mainCamera;

	vertices.clear();
	triangles.clear();
	textureBuffer.clear();

	closegraph();
}

void UT3D::clearDevice() {
	cleardevice();
}

//basic model setup functions
void UT3D::addVertex(const ver& vertex) {
	vertices.push_back(vertex);
}

void UT3D::addTriangle(int a, int b, int c) {
	triangles.push_back(tri(a, b, c));
}

void UT3D::addTexture(const char* path) {
	textureBuffer.emplace_back(Texture(path));
}

int UT3D::setReflaction(const vec3& pos, const vec3& normal) {
	float dist = pos.dot(normal); //shortest distance from the origin to the plane
	float x = -normal(0),
		y = normal(1),
		z = normal(2);

	//reflaction matrix
	mat4 reflaction;
	reflaction << 1 - 2 * x * x, -2 * x * y, -2 * x * z, -2 * x * dist,
				-2 * x * y, 1 - 2 * y * y, -2 * y * z, -2 * y * dist,
				-2 * x * z, -2 * y * z, 1 - 2 * z * z, -2 * z * dist,
				0, 0, 0, 1;

	mainCamera->setReflaction(reflaction);

	textureBuffer.emplace_back(Texture(
		mainCamera->getScreenWidth(),
		mainCamera->getScreenHeight()
	));
	return this->reflactionTextureIndex = textureBuffer.size() - 1;
}

void UT3D::setPerspective(float fov) {
	mainCamera->setPerspective(fov);
}
void UT3D::setOrthogonal(float f) {
	mainCamera->setOrthogonal(f);
}
void UT3D::setCameraPosition(const vec3& pos) {
	mainCamera->setPosition(pos);
}
void UT3D::setCameraLookat(const vec3& lookat) {
	mainCamera->setLookat(lookat);
}
void UT3D::setCameraUp(const vec3& up) {
	mainCamera->setUp(up);
}

void UT3D::setStatEnable(bool enable) {
	mainCamera->setStatEnable(enable);
}

const Stat& UT3D::getRenderStat() {
	return mainCamera->getRenderStat();
}

//add lighting into scene
void UT3D::addLighting(Light* light) {
	light->getCamera()->bindVertices(&vertices);
	light->getCamera()->bindTriangles(&triangles);
	lightings.push_back(light);
}

//return pairs of texture name and texture index
map<string, int>* _loadMTL(const string& dir, const string& path) {
	ifstream in(dir + path);
	if (in.is_open() == false) {
		in.close();
		return nullptr;
	}
	//pairs of texture path and texture index
	map<string, int>* ret = new map<string, int>();
	string name, temp; //string buffers
	char ch; //character buffer
	while (!in.eof()) {
		in >> temp;
		if (temp[0] == '#') { //skip the comments
			getline(in, temp);
			continue;
		}
		if (temp == "newmtl") { //beginning of mtl section
			in >> name;
		} else if (temp == "map_Kd") { //diffuse map
			in >> temp;
			if (ret->find(name + "map_Kd") == ret->end()){ //not been loaded yet
				UT3D::instance()->addTexture((dir + temp).c_str());
				(*ret)[name + "map_Kd"] = UT3D::instance()->textureBuffer.size();
			}
		} else if (temp == "map_Bump") { //normal map
			in >> temp;
			if (ret->find(name + "map_Bump") == ret->end()) { //not been loaded yet
				UT3D::instance()->addTexture((dir + temp).c_str());
				(*ret)[name + "map_Bump"] = UT3D::instance()->textureBuffer.size();
			}
		} else if (temp == "map_Ns") { //specular map
			in >> temp;
			if (ret->find(name + "map_Ns") == ret->end()) { //not been loaded yet
				UT3D::instance()->addTexture((dir + temp).c_str());
				(*ret)[name + "map_Ns"] = UT3D::instance()->textureBuffer.size();
			}
		} else if (temp == "") { //empty line, ignore it
			;
		} else { //not supported information
			getline(in, temp);
		}
	}

	in.close();
	return ret;
}

//the code is to be simplified
//only for .obj format
void UT3D::loadModel(const char* dir, const char* file) {
	string path = dir; //direction of model

	ifstream in(path + file);

	//store texture coordinations
	vector<float> *uvs = new vector<float>();

	//array of normals
	vector<float> *norms = new vector<float>();

	// atri = a triangle, for converting a face to triangles
	vector<int> *atri = new vector<int>();

	//for group to texture mapping
	map<string, int>* mtlmap = nullptr;
	string currentGroup;

	mat3 rotator, r;
	rotator << 1, 0, 0,
		0, 0, -1,
		0, 1, 0;/*
	r << -1, 0, 0,
		0, -1, 0,
		0, 0, 1;
	rotator = r * rotator;*/

	char ch;
	string buffer;
	int vtc = UT3D::vertices.size(), vnc = vtc,
		tc = UT3D::triangles.size(), vbase = vertices.size() - 1,
		ia, ib, ic;
	float a, b, c;
	//.obj reading
	while (!in.eof()) {
		in >> buffer;
		if (buffer[0] == '#') { //comments
			getline(in, buffer);
			continue;
		}
		if (buffer[0] == 'v') {
			if (buffer == "v") { //vertex
				in >> a >> b >> c;
				getline(in, buffer); //might have vertex color
				//magic numbers, kind of model-to-world mapping
				a *= 100.0f; b *= 100.0f; c *= 100.0f;
				vec3 pos(a, b, c);
				pos = rotator * pos;
				UT3D::addVertex(ver(pos));
			} else if (buffer == "vn") { //world space normal
				in >> a >> b >> c;
				vec3 norm(a, b, c);
				norm = rotator * norm;
				norms->push_back(norm(0));
				//rotate with vertices
				norms->push_back(norm(1));
				norms->push_back(norm(2));
			} else if (buffer == "vt") { //texture coordinate
				in >> a >> b;
				uvs->push_back(a);
				uvs->push_back(b);
				getline(in, buffer); //skip the end
			}
		} else if (buffer == "f") { //Face
			getline(in, buffer);
			int len = buffer.size();
			//format: v[/vt/vn]
			for (int i = 0;i < len;++i) {
				//skip the empty first
				if (buffer[i] == ' ') continue;
				if (buffer[i] == '\n' || buffer[i] == '\0') break; //end of line

				//vertex index
				ia = 0;
				while (buffer[i] >= '0' && buffer[i] <= '9') {
					ia = ia * 10 + (buffer[i] - '0');
					++i;
				}
				atri->push_back(ia);

				//optional, texture index
				if (buffer[i] == '/') {
					++i;
					ib = 0;
					while (buffer[i] >= '0' && buffer[i] <= '9') {
						ib = ib * 10 + (buffer[i] - '0');
						++i;
					}
					if (vertices[vbase + ia].uv(0) == 0.0f) {
						vertices[vbase + ia].uv << (*uvs)[ib * 2 - 2], (*uvs)[ib * 2 - 1];
					} else if (abs(vertices[vbase + ia].uv(0) - (*uvs)[ib * 2 - 2]) >= 0.001f) {
						//different uv indexs at the same vertex
						//create new vertex and input different uv
						vertices.push_back(vertices[vbase + ia]);
						ia = vertices.size() - 1 - vbase;
						(*atri)[atri->size() - 1] = ia;
						vertices[vbase + ia].uv << (*uvs)[ib * 2 - 2], (*uvs)[ib * 2 - 1];
					}

					//optional, normal index
					if (buffer[i] == '/') {
						++i;
						ic = 0;
						while (buffer[i] >= '0' && buffer[i] <= '9') {
							ic = ic * 10 + (buffer[i] - '0');
							++i;
						}
						vertices[vbase + ia].normal
							<< (*norms)[ic * 3 - 3],
							(*norms)[ic * 3 - 2],
							(*norms)[ic * 3 - 1];
					}
				}
			}
			if (in.eof()) break;

			//cut faces into triangles, anti-clockwise
			ia = vbase + (*atri)[0];
			for (int i = 1;i < atri->size() - 1;++i) {
				ib = vbase + (*atri)[i]; ic = vbase + (*atri)[i + 1];
				UT3D::addTriangle(ia, ib, ic);
				//texture index
				int index = (*mtlmap)[currentGroup + "map_Kd"];
				vertices[ia].texIndex = index - 1;
				vertices[ib].texIndex = index - 1;
				vertices[ic].texIndex = index - 1;
				//normal index
				index = (*mtlmap)[currentGroup + "map_Bump"];
			}
			atri->clear();
		} else if (buffer == "mtllib") { //Mtllib declaration
			in >> buffer;
			mtlmap = _loadMTL(path, buffer);
		} else if (buffer == "usemtl") { //Usemtl
			in >> buffer;
			currentGroup = buffer;
		} else if (buffer == "g") { //Group, skip it for now
			getline(in, buffer);
		} else if (buffer == "") { //empty line
			;
		} else { //s | others are skipped
			getline(in, buffer);
		}
	}

	in.close();
	if (mtlmap) {
		delete mtlmap;
	}
	delete uvs;
	delete atri;
}

void UT3D::drawPixel(int x, int y, UINT32 color) {
	putpixel_f(x, WIN_HEIGHT - 1 - y, color);
}

void UT3D::drawPixel(const ver& v) {
	//make sure no out of range error
	putpixel_f(v.position(0),
		WIN_HEIGHT - v.position(1) - 1,
		Color::toRGBValue(v.color));
}

//geometry stage pipeline, base on triangles
void UT3D::draw(float deltaTime) {
	static float t = 0.0f;
	float temp = sin(t + deltaTime) - sin(t);
	t += deltaTime;

	for (auto it = lightings.begin();
		it != lightings.end();++it) {
		//(*it)->translateBy(vec3(100 * temp, 0, 100 * temp));
		(*it)->bakeShadowmap();
	}

	if (reflactionTextureIndex != -1) {
		mainCamera->render();
		//copy camera output into texture
		textureBuffer[reflactionTextureIndex].loadFromArray(
			(const unsigned int*)mainCamera->getColorBuffer(),
			mainCamera->getScreenWidth(),
			mainCamera->getScreenHeight()
		);
		mainCamera->reflactionEnable(false);
		mainCamera->render();
		mainCamera->reflactionEnable(true);
	} else {
		mainCamera->render();
	}

	//output final render result
	const int* cptr = mainCamera->getColorBuffer();
	for (int y = 0;y < WIN_HEIGHT;++y) {
 		for (int x = 0;x < WIN_WIDTH;++x) {
			if (*cptr) drawPixel(x, y, *cptr);
			++cptr;
		}
	}

	/*
	//test output
	float color;
	Cubemap* cp = ((PointLight*)lightings[0])->depthcube;
	for (int y = 0;y < WIN_HEIGHT;++y) {
		for (int x = 0;x < WIN_WIDTH;++x) {
			color = cp->getColor(
				2.0f * (x - WIN_WIDTH / 2) / WIN_WIDTH,
				1.0f,
				2.0f * (y - WIN_HEIGHT / 2) / WIN_HEIGHT
			);
			drawPixel(x, y, color);
			++color;
		}
	}*/
}
