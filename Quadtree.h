#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Quadtree
{
	public:
		Quadtree(glm::vec2 centre, glm::vec2 size);
		~Quadtree();

		bool insert(glm::vec2 p);
		bool inBoundry(glm::vec2 p);
		void subdivide();

	protected:


		bool children;
		bool empty;

		glm::vec2 centre;
		glm::vec2 size;

		std::vector<glm::vec2> points;

		Quadtree *northWest;
		Quadtree *northEast;
		Quadtree *southWest;
		Quadtree *southEast;




	private:

};

