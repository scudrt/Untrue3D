/*
Interface class between users and basic implementation
*/

#pragma once

#include "Eigen/StdVector"
#include "graphics.h"
#include "untrue_type.h"
#include "Camera.h"
#include "Light.h"

#include <vector>

namespace untrue {
	struct UT3D {
	public:
		static UT3D* instance();

		untrue::Camera *mainCamera;

		//Basic graphics data set
		std::vector<tri> triangles;
		std::vector<ver, Eigen::aligned_allocator<ver> > vertices;
		std::vector<Texture> textureBuffer;
		std::vector<Light*> lightings;

		//basic setup functions
		void init(int width, int height, untrue::RenderMode renderMode = untrue::NORMAL);
		void onFinish();
		void clearDevice();

		void addLighting(Light*);

		//return texture index of reflaction texture
		int setReflaction(const vec3& pos, const vec3& normal);

		void setPerspective(float fov);
		void setOrthogonal(float f = 90000.0f);
		void setCameraPosition(const vec3&);
		void setCameraLookat(const vec3&);
		void setCameraUp(const vec3&);

		void setStatEnable(bool);
		const Stat& getRenderStat();

		//Graphics data managing functions
			//手动管理模型数据
		void addVertex(const ver&);
		void addTriangle(int, int, int);
		void addTexture(const char*);
		void loadModel(const char* dir, const char* filename);

		//just call it every frame after finishing all setups
		void draw(float deltaTime = 0.0f);

		//drawing section
		void drawPixel(const ver&);
		void drawPixel(int, int, UINT32);
	private:
		int WIN_HEIGHT, WIN_WIDTH;

		UT3D();
		~UT3D();

		static UT3D* __inst;

		int reflactionTextureIndex;
	};
};
