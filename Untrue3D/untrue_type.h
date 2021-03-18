#pragma once

#include "Eigen/Dense"

using mat2 = Eigen::Matrix2f;
using mat3 = Eigen::Matrix3f;
using mat4 = Eigen::Matrix4f;

using vecx = Eigen::VectorXf;
using vec4 = Eigen::Vector4f;
using vec3 = Eigen::Vector3f;
using vec3i = Eigen::Vector3f;
using vec3f = Eigen::Vector3i;
using vec2 = Eigen::Vector2f;

//will support mipmap one day
struct Texture {
	Texture(const char* path);
	Texture();
	Texture(Texture&&);
	Texture(int, int);
	Texture(const unsigned int**, int size);
	virtual ~Texture();

	//texture sampling
	unsigned int getColor(float, float);
	unsigned int getColor(const vec2&);

	void load(const unsigned int*, int);
	void loadFromPath(const char* path);
	void loadFromArray(const unsigned int* arr, int w, int h);

	int width, height;
private:
	unsigned int** colorData;
};

enum CubeFace {
	LEFT, //-x
	RIGHT, //+x
	BACK, //-y
	FRONT, //+y
	BOTTOM, //-z
	TOP, //+z
};

struct Cubemap{
	Cubemap();
	~Cubemap();

	//cubemap sampling using a 3-dimension vector
	float getColor(float, float, float);
	float getColor(const vec3&);

	void init(int size);
	void loadTextureByFace(const float* arr, CubeFace face);

	//cubemap of 6 * size * size
	int size;
	float*** cubeData;
private:

	void onDestroy();
};

struct Vertex {
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	Vertex(const vec4&, const vec3& color = vec3(1, 1, 1));
	Vertex(const vec3&, const vec3& color = vec3(1, 1, 1));
	Vertex(const vec4&, const vec2& uv);
	Vertex(const vec3&, const vec2& uv);
	Vertex();
	Vertex operator =(const Vertex&);
	vec4 position;
	vec3 color, normal;
	vec2 uv; //texture coordinate

	int texIndex;
	bool receiveShadow;

	//basic vertex calculating function
	Vertex operator -(const Vertex&)const;
	Vertex& operator -=(const Vertex&);
	Vertex operator +(const Vertex&)const;
	Vertex& operator +=(const Vertex&);
	Vertex operator *(float)const;
	Vertex& operator *=(float);
	Vertex operator /(float)const;
	Vertex& operator /=(float);
};
using ver = Vertex;

/*
anti-clockwise
*/
struct Triangle {
	Triangle();
	Triangle(int, int, int, Texture* tptr = nullptr);
	int& operator()(int _index);
	int operator()(int _index) const;

	int index[3];
};
using tri = Triangle;

struct Color {
	static int toRGBValue(const vec3&);
	static vec3 toColorVector(int);
	static void add(vec3&, const vec3&);
	static void mul(vec3&, const vec3&);
};

