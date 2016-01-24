/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_GRAPHICS_LIGHT
#define N_GRAPHICS_LIGHT

#include "Transformable.h"
#include "ShadowRenderer.h"
#include "Color.h"
#include "VarianceShadowRenderer.h"
#include "ExponentialShadowRenderer.h"
#include "BSShadowRenderer.h"
#include <n/math/Volume.h>
#include <n/utils.h>

namespace n {
namespace graphics {

class Light : public Movable
{
	public:
		Light() : Movable(), intensity(1), color(1), shadows(0) {
		}

		virtual ~Light() {
			delete shadows;
		}

		const Color<> &getColor() const {
			return color;
		}

		void setColor(const Color<> &c, bool normY = false) {
			color = normY ? c.withLightness(1) : c;
		}

		float getIntensity() const {
			return intensity;
		}

		void setIntensity(float t) {
			intensity = t;
		}

		ShadowRenderer *getShadowRenderer() const {
			return shadows;
		}

		bool castShadows() const {
			return shadows;
		}

	private:
		float intensity;
		Color<> color;

	protected:
		ShadowRenderer *shadows;
};

class DirectionalLight : public Light
{
	public:
		DirectionalLight() : Light() {
		}

		virtual ~DirectionalLight() {
		}
};

class BoxLight : public Light
{
	public:
		BoxLight(const math::Vec3 &s = math::Vec3(10)) : Light(), size(s) {
		}

		virtual ~BoxLight() {
		}

		math::Vec3 getSize() const {
			return size * getScale();
		}

		void setSize(const math::Vec3 &s) {
			size = s;
		}

		template<typename T = BSShadowRenderer, typename... Args>
		void setCastShadows(const Scene *sc, uint res = 512, Args... args) {
			delete shadows;
			shadows = 0;
			if(sc) {
				shadows = new T(new BoxLightShadowRenderer(this, sc, res), args...);
			}
		}

		void setCastUnfilteredShadows(const Scene *sc, uint res = 512) {
			delete shadows;
			shadows = 0;
			if(sc) {
				shadows = new BoxLightShadowRenderer(this, sc, res);
			}
		}

	private:
		math::Vec3 size;
};


class PointLight : public Light
{
	public:
		PointLight(float r = 1) : Light() {
			Transformable::radius = r;
		}

		virtual ~PointLight() {
		}

		void setRadius(float r) {
			radius = r;
		}

};

class SpotLight : public Light
{
	public:
		SpotLight(float r = 1, float ang = math::pi * 0.5, float e = 0.5) : Light(), angle(ang), exp(e) {
			Transformable::radius = r;
		}

		virtual ~SpotLight() {
		}

		float getCutOff() const {
			return angle;
		}

		void setCutOff(float ang) {
			angle = ang;
		}

		void setRadius(float r) {
			radius = r;
		}

		void setExponent(float e) {
			exp = e;
		}

		float getExponent() const {
			return exp;
		}

		template<typename T = BSShadowRenderer, typename... Args>
		void setCastShadows(const Scene *sc, uint res = 128, Args... args) {
			delete shadows;
			shadows = 0;
			if(sc) {
				shadows = new T(new SpotLightShadowRenderer(this, sc, res), args...);
			}
		}

		void setCastUnfilteredShadows(const Scene *sc, uint res = 128) {
			delete shadows;
			shadows = 0;
			if(sc) {
				shadows = new SpotLightShadowRenderer(this, sc, res);
			}
		}

	private:
		float angle;
		float exp;
};

}
}


#endif // LIGHT

