#include "untrue_type.h"
#include "graphics.h"

#include <fstream> //texture loading

//VERTEX
Vertex::Vertex(const vec4& pos, const vec3& color)
	: position(pos), color(color){
	normal.setZero();
	uv.setZero();
	texIndex = -1;
	receiveShadow = false;
}
Vertex::Vertex(const vec3& pos, const vec3& color)
	: color(color){
	position << pos, 1;
	normal.setZero();
	uv.setZero();
	texIndex = -1;
	receiveShadow = false;
}
Vertex::Vertex(const vec4& pos, const vec2& uv)
	:position(pos), uv(uv){
	color.setZero();
	normal.setZero();
	texIndex = -1;
	receiveShadow = false;
}
Vertex::Vertex(const vec3& pos, const vec2& uv)
	: uv(uv) {
	position << pos, 1;
	color.setZero();
	normal.setZero();
	texIndex = -1;
	receiveShadow = false;
}
Vertex::Vertex(){
	texIndex = -1;
	receiveShadow = false;
}
Vertex Vertex::operator =(const Vertex& other) {
	position = other.position;
	color = other.color;
	normal = other.normal;
	uv = other.uv;
	texIndex = other.texIndex;
	receiveShadow = other.receiveShadow;
	return *this;
}
//redefinitions of vertex operation
Vertex& Vertex::operator -=(const Vertex& other) {
	position -= other.position;
	color -= other.color;
	normal -= other.normal;
	uv -= other.uv;
	texIndex = other.texIndex;
	receiveShadow = other.receiveShadow;
	return *this;
}
Vertex& Vertex::operator +=(const Vertex& other) {
	position += other.position;
	color += other.color;
	normal += other.normal;
	uv += other.uv;
	texIndex = other.texIndex;
	receiveShadow = other.receiveShadow;
	return *this;
}
Vertex& Vertex::operator *=(float k) {
	position *= k;
	color *= k;
	normal *= k;
	uv *= k;
	return *this;
}
Vertex& Vertex::operator /=(float k) {
	position /= k;
	color /= k;
	normal /= k;
	uv /= k;
	return *this;
}
Vertex Vertex::operator - (const Vertex& other)const {
	Vertex temp(*this);
	return temp -= other;
}
Vertex Vertex::operator +(const Vertex& other)const {
	Vertex temp (*this);
	return temp += other;
}
Vertex Vertex::operator *(float k)const {
	Vertex temp (*this);
	return temp *= k;
}
Vertex Vertex::operator /(float k)const {
	Vertex temp (*this);
	return temp /= k;
}

//TRIANGLE
Triangle::Triangle(int a, int b, int c, Texture* tptr) {
	index[0] = a;
	index[1] = b;
	index[2] = c;
}

Triangle::Triangle() {
	;
}

//Eigen-style data access
int& Triangle::operator()(int _index) {
	return index[_index];
}
int Triangle::operator()(int _index) const{
	return index[_index];
}

Texture::Texture() {
	;
}

//create a black texture of indicated size
Texture::Texture(int width, int height) {
	this->width = width;
	this->height = height;

	this->colorData = new unsigned int*[height];
	this->colorData[0] = new unsigned int[height * width];
	for (int i = 1;i < height;++i) {
		this->colorData[i] = this->colorData[i - 1] + width;
	}
	memset(this->colorData[0], 0, sizeof(unsigned int) * width * height);
}

Texture::Texture(Texture&& tex) {
	this->width = tex.width;
	this->height = tex.height;
	this->colorData = tex.colorData;
	tex.colorData = nullptr;
}

//load texture by using EGE's methods
Texture::Texture(const char* path) {
	loadFromPath(path);
}

Texture::Texture(const unsigned int** map, int size) {
	load(map[0], size);
}

Texture::~Texture() {
	if (colorData) {
		delete[] colorData[0];
		delete[] colorData;
	}
}

void Texture::load(const unsigned int* map, int size) {
	this->width = size;
	this->height = size;

	//Memory allocating
	this->colorData = new unsigned int*[size];
	this->colorData[0] = new unsigned int[size * size];
	for (int i = 1;i < size;++i) colorData[i] = colorData[i - 1] + size;

	//Deep copy from image object
	const unsigned int *tp = map;
	for (int i = 0;i < size;++i) {
		for (int j = 0;j < size;++j) {
			colorData[size - 1 - i][j] = (unsigned int)tp[j + i * size];
		}
	}
}

void Texture::loadFromPath(const char* path) {
	//Loading image
	PIMAGE img = newimage();
	assert(getimage(img, path) == grOk);
	load((const unsigned int*)getbuffer(img), getwidth(img));
	delimage(img);
}

void Texture::loadFromArray(const unsigned int* arr, int w, int h) {
	this->width = w;
	this->height = h;

	if (this->colorData) {
		delete[] this->colorData[0];
		delete[] this->colorData;
	}
	this->colorData = new unsigned int*[height];
	this->colorData[0] = new unsigned int[height * width];

	memcpy(colorData[0], arr, sizeof(unsigned int) * width);
	for (int i = 1;i < height;++i) {
		colorData[i] = colorData[i - 1] + width;
		memcpy(colorData[i], arr + i * width, sizeof(unsigned int) * width);
	}
}

//start from left and down
unsigned int Texture::getColor(float u, float v) {
	u = min(u, 1.0f); u = max(u, 0.0f);
	v = min(v, 1.0f); v = max(v, 0.0f);
	return colorData[int(v * (height - 1))][int(u * (width - 1))];
}

unsigned int Texture::getColor(const vec2& uv) {
	return getColor(uv(0), uv(1));
}

Cubemap::Cubemap() {
	size = 0;
	cubeData = nullptr;
}

Cubemap::~Cubemap() {
	onDestroy();
}

void Cubemap::onDestroy() {
	if (this->cubeData) {
		//detele every face
		for (int i = 0;i < 6;++i) {
			delete[] cubeData[i][0];
			delete[] cubeData[i];
		}
		delete[] cubeData;
	}
}

float Cubemap::getColor(float x, float y, float z) {
	return getColor(vec3(x, y, z));
}

float Cubemap::getColor(const vec3& v) {
	//get index of the max coefficient
	int k = 0;
	if (abs(v(k)) < abs(v(1))) k = 1;
	if (abs(v(k)) < abs(v(2))) k = 2;

	//take the other two coefficients as uv
	vec2 uv;
	for (int i = 0, index = 0;i < 3;++i) {
		if (i != k) {
			uv(index) = v(i);
			++index;
		}
	}
	float len = v(k);
	uv /= abs(len); //map to [-1, 1]
	uv = (uv + vec2(1, 1)) / 2.0f; //map to [0, 1]

	//face index of cubemap
	k = (k << 1) + (len >= 0 ? 1 : 0);

	return this->cubeData[k][int(uv(1) * (size - 1))][int(uv(0) * (size - 1))];
}

//this function is made to set the size of cubemap previously
void Cubemap::init(int size) {
	//check if the cubemap array was created, avoid memory leaking
	onDestroy();

	this->size = size;

	//creating a cubemap, containing 6 * size * size pixels
	this->cubeData = new float**[6];
	for (int i = 0;i < 6;++i) {
		this->cubeData[i] = new float*[size];
		this->cubeData[i][0] = new float[size * size];
		for (int j = 1;j < size;++j) {
			this->cubeData[i][j] = this->cubeData[i][j - 1] + size;
		}
	}
}

//copy the texture into the cubemap, Cubemap::init(int) must be called firstly
void Cubemap::loadTextureByFace(const float* arr, CubeFace face) {
	memcpy(
		//face is an enum variable, its value represents cubemap texture index
		this->cubeData[(int)face][0],
		arr,
		this->size * this->size * sizeof(float)
	);
}

int Color::toRGBValue(const vec3& c) {
	return (int(c(0) * 255) << 16)
		| (int(c(1) * 255) << 8)
		| int(c(2) * 255);
}

vec3 Color::toColorVector(int rgb) {
	return vec3(
		(rgb >> 16) & 255,
		(rgb >> 8) & 255,
		rgb & 255
	) / 255.0f;
}

//do color adding and ensure it won't be out of value range
void Color::add(vec3& color, const vec3& delta) {
	color += delta;
	color(0) = max(0.0f, min(1.0f, color(0)));
	color(1) = max(0.0f, min(1.0f, color(1)));
	color(2) = max(0.0f, min(1.0f, color(2)));
}

void Color::mul(vec3& color, const vec3& delta) {
	color(0) = max(0.0f, min(1.0f, color(0) * delta(0)));
	color(1) = max(0.0f, min(1.0f, color(1) * delta(1)));
	color(2) = max(0.0f, min(1.0f, color(2) * delta(2)));
}
