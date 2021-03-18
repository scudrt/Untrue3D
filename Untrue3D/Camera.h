#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

#include "untrue_type.h"

namespace untrue {

	enum RenderMode {
		NORMAL,
		WIREFRAME,
		DEPTH //for shadow map
	};

	enum ThreadState {
		RUNNING,
		SUSPENDING,
		EXIT
	};

	struct Stat {
		float geometryTime;
		float rasterizationTime;
		float lightingTime;
		int renderingFaces;
	};

	class Camera {
	public:
		Camera();
		~Camera();

		void bindVertices(std::vector<ver, Eigen::aligned_allocator<ver> >* vs);
		void bindTriangles(std::vector<tri>* ts);

		void render();

		//Convert coordinate from screen space to world space
		void screenToWorld(vec4&);

		//Convert coordinate from world space to screen space
		void worldToScreen(vec4&);

		//Convert normal vector from world to camera space
		void normalWorldToCamera(vec3&);

		void setCamera(int, int, RenderMode);

		//set plane reflaction matrix
		void setReflaction(const mat4&);
		void reflactionEnable(bool enable);

		void setPosition(const vec3&);
		void setLookat(const vec3&);
		void setUp(const vec3&);

		void translateBy(const vec3&);

		void rotateBy(const vec3&);

		void setPerspective(float fov, float n = 1.0f, float f = 90000.0f);

		void setOrthogonal(float depth = 90000.0f);

		float* getDepthBuffer();
		ver* getFragBuffer();
		const int* getColorBuffer();

		int getScreenWidth();
		int getScreenHeight();

		void setStatEnable(bool);
		const Stat& getRenderStat();

		void setBackCulling(bool);

		const vec3& getPosition();
		//return normalized camera direction
		const vec3& getDirection();

		const RenderMode& getRenderMode();

	private:
		const int RASTHREAD_SIZE = 5;
		const int FRAMETHREAD_SIZE = 16;

		float fov, n, f;

		Stat renderStat;

		float** depthBuffer;

		ver** frags;

		int** colorBuffer;

		int screenWidth, screenHeight;

		//to check if need perspective division
		bool isPerspective;

		//statictics switch
		bool isStatEnable;

		bool isBackCulling;

		bool reflactionEnabled;

		RenderMode renderMode;

		vec3 position,
			lookat,
			up,
			rotation,
			direction;

		mat4 reflaction,
			viewTransform,
			projection,
			screenMapping;

		//matrix cache
		mat4 _worldToCVV,
			_CVVToWorld;

		//store CVV-clipped triangles
		std::vector<tri> tBuffer,
			*ts;

		//store transformed vertices
		std::vector<ver, Eigen::aligned_allocator<ver> > vBuffer,
			*vs;

		//multi-threading
		std::thread *__rasthreads, *__framethreads;
		std::mutex* depthLock;
		volatile ThreadState* __rasthreadState, *__framethreadState;

		//threads synchronizing
		std::mutex rasmutex, framemutex; //mutex for critical section while rendering
		std::condition_variable threadRasCon, mainRasCon, threadFrameCon, mainFrameCon;
		std::atomic_int rasFinished, renderingFace, frameFinished;
		std::atomic_bool *rasReady, *frameReady;

		mat3 getRotation();

		bool isBackward(const tri& t);
		bool isBackward(const ver&, const ver&, const ver&);

		void updateCameraState();

		//Clippings
		int _getClipCode(const vec4&);
		void nearClip(ver&, ver&);

		void triangleClip(tri);
		bool lineClip(vec2&, vec2&);
		int clipcode2d(const vec2&);

		//parallel algorithms
		void rasterizationThread(int);

		void frameThread(int);

		void rasterizeTriangle(const tri&);

		void drawLine(const ver&, const ver&);
		void wireframeTriangle(const tri&);

		void __initThreads();

		void __killThreads();

		void __resumeAllThreads(const char* type);

		void __waitForAllThreads(const char* type);
	};

};
