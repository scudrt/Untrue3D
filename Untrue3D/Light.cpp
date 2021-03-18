#include "Light.h"

#include <algorithm>

using namespace untrue;

Light::Light(bool isStatic, int size){
	isShadowmapBaked = false;

	this->isStatic = isStatic;

	lightCamera = new Camera();
	lightCamera->setCamera(size, size, DEPTH);
	lightCamera->setPosition(vec3(0, 0, 0));
	lightCamera->setLookat(vec3(0, 1, 0));
	lightCamera->setUp(vec3(0, 0, 1));

	this->shadowmapSize = size;
	this->halfSize = size >> 1;

	this->intensity = 16.0f;
	this->lightColor = vec3::Ones();
}

Light::~Light() {
	delete lightCamera;
}

void Light::setPosition(const vec3& pos) {
	lightCamera->setPosition(pos);
}

void Light::translateBy(const vec3& delta) {
	lightCamera->translateBy(delta);
}

void Light::rotateBy(const vec3& rotation) {
	lightCamera->rotateBy(rotation);
}

void Light::setShadowmapSize(int size) {
	this->shadowmapSize = size;
	this->halfSize = size >> 1;
	lightCamera->setCamera(size, size, DEPTH);
}

Camera* Light::getCamera() {
	return lightCamera;
}

void Light::setColor(const vec3& color) {
	this->lightColor = color;
}

vec3 Light::getColor() {
	return this->lightColor;
}

void Light::setIntensity(float intensity) {
	this->intensity = intensity;
}

//simple shadowmap baking logic for spot light and directional light
void Light::bakeShadowmap() {
	if (isStatic && isShadowmapBaked) return;
	lightCamera->render();
	isShadowmapBaked = true;
}

/*** SPOT LIGHT ***/
SpotLight::SpotLight(bool isStatic, int size)
	: Light(isStatic, size){
	lightCamera->setPerspective(90.0f);
}

/*
* input: fragment data, eye direction
* return light intensity of the given fragment
*/
float SpotLight::light(const ver& frag, const vec3& eyedir) {
	float product = this->intensity, ret = 0.20f;
	//return ambient value if fragment is in shadow
	if (frag.receiveShadow && shadow(frag) > 0) return ret;

	//Diffuse, according to Blinn-Phong method
	vec3 v(frag.position.head(3));
	v = lightCamera->getPosition() - v;
	product *= 10000.0f / v.dot(v); // transfer distance from cm^2 to m^2
	v.normalize(); //light direction
	float temp = frag.normal.dot(v);
	ret += product * std::max(0.0f, temp);

	//Specular, according to Blinn-Phong method
	v += eyedir; v.normalize(); //half vector
	temp = frag.normal.dot(v);
	temp = std::max(0.0f, temp);
	//Ns = 16, magic number
	temp *= temp; temp *= temp; temp *= temp;
	ret += product * temp * temp;

	return ret;
}

/*
* input: fragment data
* return: whether the fragment is in shadow, 1 or 0
* using some magic numbers
*/
float SpotLight::shadow(const ver& frag) {
	vec4 pos(frag.position);
	lightCamera->worldToScreen(pos);
	int x = pos(0), y = pos(1);

	//out of range
	if ((x | y) < 0 || x >= shadowmapSize || y >= shadowmapSize) return 1.0f;
	float z = pos(3);
	if (z <= 0.0f) return 1.0f;

	//check if the pixel is inside the lighting circle
	int xx = x - halfSize, yy = y - halfSize;
	if (xx * xx + yy * yy > halfSize * halfSize) return 1.0f;

	//calculate normalized lighting direction
	pos.head(3) = frag.position.head(3) - lightCamera->getPosition();
	pos(3) = 0.0f;
	pos.normalize();

	//slope based shadow bias with magic numbers
	float bias = frag.normal.dot(pos.head(3));
	if (abs(bias) < 0.00001f) return 1.0f;
	bias = -0.0024 / bias;
	bias = std::max(-z / 20000.0f, bias);
	bias = std::min(bias, z / 20000.0f);
	return z * (1.0 - bias) <= lightCamera->getDepthBuffer()[y * shadowmapSize + x] ? 0.0f : 1.0f;
}

/*** DIRECTIONAL LIGHT ***/
DirectionalLight::DirectionalLight(bool isStatic, int size)
	: Light(isStatic, size){
	lightCamera->setOrthogonal();
}

float DirectionalLight::light(const ver& frag, const vec3& eyedir) {
	float product = this->intensity, ret = 0.20f;
	//return ambient value if fragment is in shadow
	if (frag.receiveShadow && shadow(frag) > 0) return ret;

	//diffuse, Id = I / (r^2) * max(0, n * l)
	vec3 v(frag.position.head(3));
	v = lightCamera->getPosition() - v;
	product *= 10000.0f / v.dot(v); // transfer distance from cm^2 to m^2
	v = -lightCamera->getDirection(); //directional light has only one direction
	float temp = frag.normal.dot(v);
	ret += product * std::max(0.0f, temp);

	//specular, Is = I / (r^2) * max(0, n * h)^Ns
	v += eyedir; v.normalize(); //half vector
	temp = frag.normal.dot(v);
	temp = std::max(0.0f, temp);
	//Ns = 16
	temp *= temp; temp *= temp; temp *= temp;
	ret += product * temp * temp;

	return ret;
}

float DirectionalLight::shadow(const ver& frag) {
	vec4 pos(frag.position);
	lightCamera->worldToScreen(pos);
	int x = pos(0), y = pos(1);

	//out of range
	if ((x | y) < 0 || x >= shadowmapSize || y >= shadowmapSize) return 1.0f;
	float z = pos(3);
	if (z <= 0.0f) return 1.0f;

	float bias = frag.normal.dot(lightCamera->getDirection());
	if (abs(bias) < 0.00001f) return 1.0f;

	bias = -0.0024 / bias;
	bias = std::max(-z / 20000.0f, bias);
	bias = std::min(bias, z / 20000.0f);
	bias += 3.0 / z;

	return z * (1.0 - bias) <= lightCamera->getDepthBuffer()[y * shadowmapSize + x] ? 0.0f : 1.0f;
}

/*** POINT LIGHT***/
PointLight::PointLight(bool isStatic, int size)
	: Light(isStatic, size) {
	lightCamera->setPerspective(90.0f);
	depthcube = new Cubemap();
	depthcube->init(size);
}

PointLight::~PointLight() {
	if (depthcube) delete depthcube;
}

//bake cubemap for point light
void PointLight::bakeShadowmap() {
	if (isStatic && isShadowmapBaked) return;
	//render every sides for the cubemap
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), FRONT);
	lightCamera->rotateBy(vec3(0, 0, 90));
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), LEFT);
	lightCamera->rotateBy(vec3(0, 0, 90));
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), BACK);
	lightCamera->rotateBy(vec3(0, 0, 90));
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), RIGHT);
	lightCamera->rotateBy(vec3(90, 0, 90));
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), TOP);
	lightCamera->rotateBy(vec3(-180, 0, 0));
	lightCamera->render();
	depthcube->loadTextureByFace(lightCamera->getDepthBuffer(), BOTTOM);
	lightCamera->rotateBy(vec3(90, 0, 0)); //back to the front face
	isShadowmapBaked = true;
}

void PointLight::setShadowmapSize(int size) {
	Light::setShadowmapSize(size);
	depthcube->init(size);
}

float PointLight::light(const ver& frag, const vec3& eyedir) {
	float product = this->intensity, ret = 0.20f;
	//return ambient value if fragment is in shadow
	if (frag.receiveShadow && this->shadow(frag) > 0) return ret;

	//Diffuse, according to Blinn-Phong method
	vec3 v(frag.position.head(3));
	v = lightCamera->getPosition() - v;
	product *= 10000.0f / v.dot(v); // transfer distance from cm^2 to m^2
	v.normalize(); //light direction
	float temp = frag.normal.dot(v);
	ret += product * std::max(0.0f, temp);

	//Specular, according to Blinn-Phong method
	v += eyedir; v.normalize(); //half vector
	temp = frag.normal.dot(v);
	temp = std::max(0.0f, temp);
	//Ns = 16, magic number
	temp *= temp; temp *= temp; temp *= temp;
	ret += product * temp * temp;

	return ret;
}

//TODO: use cubemap to sample the depth
//point light use cubemap as shadowmap
float PointLight::shadow(const ver& frag) {
	//depth of fragment
	vec3 pos = frag.position.head(3) - lightCamera->getPosition();
	float z = pos.maxCoeff();
	z = std::max(-pos.minCoeff(), z);

	//shadow bias with some magic numbers
	pos.normalize();
	float bias = frag.normal.dot(pos);
	if (abs(bias) < 0.00001f) return 1.0f;
	bias = -0.0024 / bias;
	bias = std::max(-z / 100000.0f, bias);
	bias = std::min(bias, z / 100000.0f);
	bias -= 1.6f / z;
	return z * (1.0f - bias) <= depthcube->getColor(pos.head(3)) ? 0.0f : 1.0f;
}
