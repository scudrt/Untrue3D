#pragma once

#include "Camera.h"

namespace untrue {
	class Light{
	public:
		Light(bool isStatic, int size);
		virtual ~Light();

		//Main Functions
		//return light intensity of the fragment
		virtual float light(const ver&, const vec3&) = 0;
		//return whether the fragment is in shadow (1 or 0)
		virtual float shadow(const ver&) = 0;

		//light transform
		virtual Camera* getCamera();
		virtual void rotateBy(const vec3&);
		virtual void setPosition(const vec3&);
		virtual void translateBy(const vec3&);

		virtual void bakeShadowmap();
		virtual void setShadowmapSize(int);

		//light color and intensity
		virtual vec3 getColor();
		virtual void setColor(const vec3& color);
		virtual void setIntensity(float intensity);

	protected:
		int shadowmapSize,
			halfSize; //shadowmapSize / 2

		bool isStatic;

		bool isShadowmapBaked;

		float intensity;

		Camera* lightCamera;

		vec3 lightColor;
	private:
	};

	class SpotLight : public Light {
	public:
		SpotLight(bool isStatic, int size);

		virtual float light(const ver&, const vec3&);
		virtual float shadow(const ver&);
	};

	class DirectionalLight : public Light {
	public:
		DirectionalLight(bool isStatic, int size);

		virtual float light(const ver&, const vec3&);
		virtual float shadow(const ver&);
	};

	class PointLight : public Light {
	public:
		PointLight(bool isStatic, int size);
		virtual ~PointLight();

		virtual float light(const ver&, const vec3&);
		virtual float shadow(const ver&);

		virtual void bakeShadowmap();
		virtual void setShadowmapSize(int);

		Cubemap* depthcube;
	private:
	};
};
