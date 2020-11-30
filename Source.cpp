#include "EasyBMP.hpp"
#include "Sphere.h"
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <chrono>

using namespace std::chrono;
using namespace std;
using namespace EasyBMP;


#define MAX_RECURSION_DEPTH 5 


float mix(const float& a, const float& b, const float& mix)
{
	return b * mix + a * (1 - mix);
}

unsigned width = 2560, height = 1440;
string filename = "example2560x1440.bmp";

vector3 trace(const vector3& origin, const vector3& direction, sphere* spheres, const int spheresCount, const int& depth) {
float tClosest = INFINITY;
	const sphere* sphere = NULL;
	for (unsigned i = 0; i < spheresCount; ++i) {
		float t0 = INFINITY, t1 = INFINITY;
		if (spheres[i].intersect(origin, direction, t0, t1)) {
			if (t0 < 0)
				t0 = t1;
			if (t0 < tClosest) {
				tClosest = t0;
				sphere = &spheres[i];
			}
		}
	}

	if (!sphere) 
		return vector3(1);
	vector3 surfaceColor(0);
	vector3 hitPoint = origin + direction * tClosest;
	vector3 normHitPoint = hitPoint - sphere->center;
	normHitPoint.normalize();

	float bias = 1e-4;
	if (direction.dot(normHitPoint) > 0)
		normHitPoint = -normHitPoint;
	if ((sphere->reflection > 0) && depth < MAX_RECURSION_DEPTH) {
		float faceRatio = -direction.dot(normHitPoint);
		float reflectionCoef = 0.9 * (1 - faceRatio) * (1 - faceRatio) * (1 - faceRatio) + 0.1;
		vector3 reflDirection = direction - normHitPoint * 2 * direction.dot(normHitPoint);
		reflDirection.normalize();
		vector3 reflection = trace(hitPoint + normHitPoint * bias, reflDirection, spheres, spheresCount, depth + 1);
		surfaceColor = (reflection * reflectionCoef) * sphere->surfaceColor;
	}
	else {
		for (unsigned i = 0; i < spheresCount; ++i) {
			if (spheres[i].emissionColor.length() > 0) {
				vector3 doTransmit = 1;
				vector3 lightDirection = spheres[i].center - hitPoint;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheresCount; ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j].intersect(hitPoint + normHitPoint * bias, lightDirection, t0, t1)) {
							doTransmit = 0;
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * doTransmit * fmax(float(0), normHitPoint.dot(lightDirection)) * spheres[i].emissionColor;
			}
		}
	}

	return surfaceColor + sphere->emissionColor;
}


vector3* render(sphere* spheres,int sphereCount)
{
	vector3* image = new vector3[width * height], * pixel = image;
	float invWidth = 1 / float(width), invHeight = 1 / float(height);
	float fov = 30, aspectratio = width / float(height);
	float angle = tan(M_PI * 0.5 * fov / 180.0);
	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			float x_ = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
			float y_ = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
			vector3 raydir(x_, y_, -1);
			raydir.normalize();
			image[y*width+x] = trace(vector3(0), raydir, spheres, sphereCount, 0);
		}
	}

	return image;
}

void preset_1(sphere* h_spheres) {
	h_spheres[0] = sphere(vector3(0.0, -7010, -20), 7000, vector3(0.20, 0.20, 0.20), 1);
	h_spheres[1] = sphere(vector3(0.0, 0, -20), 4, vector3(0.70, 0.52, 0.16), 0.8);
	h_spheres[2] = sphere(vector3(2.0, -1, -10), 2, vector3(0.50, 0.36, 0.76), 0.7);
	h_spheres[3] = sphere(vector3(5.0, 2, -30), 3, vector3(0.55, 0.97, 0.97), 0.5);
	h_spheres[4] = sphere(vector3(-1.5, -1, -40), 1, vector3(0.10, 0.20, 0.30), 0.3);
	h_spheres[5] = sphere(vector3(-5.5, 3, -15), 3, vector3(0.90, 0.90, 0.90), 0.05);
	h_spheres[6] = sphere(vector3(0.0, 5, -30), 70, vector3(0.00, 0.00, 0.00), 0, vector3(1));
	h_spheres[7] = sphere(vector3(0.0, 2, 30), 130, vector3(0.00, 0.00, 0.00), 0, vector3(0.2, 5, 0));
}


int main(int argc, char** argv)
{
	srand(13);
	sphere* h_spheres = (sphere*)malloc(8 * sizeof(sphere));
	preset_1(h_spheres);
	cout << "Start render \n";
	auto start = high_resolution_clock::now();
	vector3* image = render(h_spheres,8);
	auto end = high_resolution_clock::now();
	cout << "Rendering complete, now drawing \n";
	Image img(width, height, filename);
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			int r = min(float(1), image[j * width + i].x) * 255;
			int g = min(float(1), image[j * width + i].y) * 255;
			int b = min(float(1), image[j * width + i].z) * 255;
			img.SetPixel(i, j, RGBColor(r, g, b));
		}
	}
	img.Write();

	delete[] image;
	auto duration = duration_cast<milliseconds>(end - start);
	cout << "Time elapsed:" << duration.count() << "ms\n";
	system("pause");
	return 0;
}