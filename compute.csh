#version 430 core
#define MAX_SCENE_BOUNDS 100.0

struct cube {
  vec3 min;
  vec3 max;
};

struct hitinfo {
  vec2 lambda;
  vec3 cubeMin;
  vec3 cubeMax;
  int bi;
};

struct Tri {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 norm;
	vec3 tex0;
	vec3 tex1;
	vec3 tex2;
};

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray01;
uniform vec3 ray10;
uniform vec3 ray11;
uniform vec3 lightPos;
uniform int NUM_CUBES;
uniform int NUM_TRIANGLES;

layout(binding = 0, rgba32f) uniform writeonly image2D framebuffer;
layout(binding = 1, rgba32f) uniform readonly image2D modelTex;
layout(std430, binding = 2) buffer cubes {
	 cube data[];
};
layout(std430, binding = 3) buffer triangles {
	Tri triData[];
};

float intersectTri(vec3 origin, vec3 dir, const Tri tri, out vec2 tex)
{
	vec3 v0v1 = tri.v1 - tri.v0;
	vec3 v0v2 = tri.v2 - tri.v0;
	vec3 pvec = cross(dir, v0v2);
	float det = dot(v0v1, pvec);

	if(abs(det) < 1e-8)
	{
		return -1;
	}

	float invDet = 1/det;

	vec3 tvec = origin - tri.v0;
	float u = dot(tvec, pvec) * invDet;
	if(u < 0 || u > 1)
	{
		return -1;
	}

	vec3 qvec = cross(tvec, v0v1);
	float v = dot(dir, qvec) * invDet;
	if(v < 0 || u + v > 1)
	{
		return -1;
	} 

	//If the texture co-ords are less than 0
	//then there is no texture information
	if(tri.tex0.x > 0)
	{
		vec3 temp = u*tri.tex0 + v*tri.tex1 + (1-u-v)*tri.tex2;
		tex.x = temp.x;
		tex.y = temp.y;
	}
	else
	{
		tex.x = tri.tex0.x;
		tex.y = tri.tex0.y;
	}

	float t = dot(v0v2, qvec) * invDet;

	return t;

}

bool intersectTriangles(vec3 origin, vec3 dir, out Tri triFound, out float smallest, out vec2 tex)
{
	smallest = MAX_SCENE_BOUNDS;
	bool found = false;
	for(int i=0; i < NUM_TRIANGLES; i++)
	{
		
		float t = intersectTri(origin, dir, triData[i], tex);
		if( t >= 0 && t < smallest)
		{
			smallest = t;
			triFound = triData[i];
			found = true;
		}
	}

	return found;
}

vec2 intersectCube(vec3 origin, vec3 dir, const cube c) 
{
  vec3 tMin = (c.min - origin) / dir;
  vec3 tMax = (c.max - origin) / dir;
  vec3 t1 = min(tMin, tMax);
  vec3 t2 = max(tMin, tMax);
  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);
  return vec2(tNear, tFar);
}

bool intersectCubes(vec3 origin, vec3 dir, out hitinfo info) 
{
  float smallest = MAX_SCENE_BOUNDS;
  bool found = false;
  for (int i = 0; i < NUM_CUBES; i++) 
  {
    vec2 lambda = intersectCube(origin, dir, data[i]);
    if (lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) 
	{
      info.lambda = lambda;
      info.bi = i;
	  info.cubeMin = data[i].min;
	  info.cubeMax = data[i].max;
      smallest = lambda.x;
      found = true;
    }
  }
  return found;
}

vec4 trace(vec3 origin, vec3 dir) 
{
	hitinfo i;
	if (intersectCubes(origin, dir, i)) 
	{
		vec3 intersect = origin + dir * i.lambda.x;
		vec3 minResult = abs(i.cubeMin - intersect);
		vec3 maxResult = abs(i.cubeMax - intersect);
		vec3 faceNormal = vec3(0, 0, 0);
		if(minResult.x < 0.01)
		{
			faceNormal = vec3(-1, 0, 0);
		}
		else if(minResult.y < 0.01)
		{
			faceNormal = vec3(0, -1, 0);
		}
		else if(minResult.z < 0.01)
		{
			faceNormal = vec3(0, 0, -1);
		}
		else if(maxResult.x < 0.01)
		{
			faceNormal = vec3(1, 0, 0);
		}
		else if(maxResult.y < 0.01)
		{
			faceNormal = vec3(0, 1, 0);
		}
		else if(maxResult.z < 0.01)
		{
			faceNormal = vec3(0, 0, 1);
		}

		// Ambient
		vec3 lightColour = vec3(1, 1, 1);
		vec3 objectColour = vec3(1, 0, 0);
		float ambientStrength = 0.3f;
		vec3 ambient = ambientStrength * lightColour;

		// Diffuse 
		vec3 norm = normalize(faceNormal);
		vec3 lightDir = normalize(lightPos - intersect);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diff * lightColour;

		vec3 result = (ambient + diffuse) * objectColour;
		vec4 colour = vec4(result, 1.0f);

		return colour;
    
	}

	Tri triFound;
	float t;
	vec2 texCoord;
	if(intersectTriangles(origin, dir, triFound, t, texCoord))
	{
		vec3 faceNormal = triFound.norm;
		vec3 intersect = origin + dir * t;
		// Ambient
		vec3 lightColour = vec3(1, 1, 1);
		float ambientStrength = 0.3f;
		vec3 ambient = ambientStrength * lightColour;

		// Diffuse 
		vec3 lightDir = normalize(lightPos - intersect);
		float diff = max(dot(faceNormal, lightDir), 0.0);
		vec3 diffuse = diff * lightColour;

		vec3 result = clamp(ambient + diffuse, 0, 1);
		vec4 colour = vec4(result, 1.0f);

		ivec2 texSize = imageSize(modelTex);
		if(texCoord.x > 0 && texSize.x > 0)
		{
			ivec2 intTexCoord = ivec2(texSize.x * texCoord.x, texSize.y * texCoord.y);
			colour = colour * imageLoad(modelTex, intTexCoord);
		}
		else
		{
			//We don't have texture information so 
			//paint object a nice shade of red
			colour = colour * vec4(0.8, 0, 0, 1);
		}

		return colour;
	}

	return vec4(0.5, 0.5, 0.5, 1.0);
}

layout (local_size_x = 16, local_size_y = 8) in;
void main(void) 
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pix.x >= size.x || pix.y >= size.y) 
	{
		return;
	}
	vec2 pos = vec2(pix) / vec2(size.x - 1, size.y - 1);
	vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);
	vec4 color = trace(eye, dir);
	imageStore(framebuffer, pix, color);
}
