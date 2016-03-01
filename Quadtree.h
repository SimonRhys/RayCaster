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
		bool inBoundry(glm::vec2 p, glm::vec2 c, glm::vec2 s);
		bool boundryIntersect(glm::vec2 c1, glm::vec2 s1, glm::vec2 c2, glm::vec2 s2);
		bool search(glm::vec2 p);
		
		std::vector<glm::vec2> search(glm::vec2 c, glm::vec2 s);

		glm::vec2 getCentre();
		glm::vec2 getSize();

		void subdivide();

	protected:


		bool leaf;

		glm::vec2 centre;
		glm::vec2 size;

		std::vector<glm::vec2> points;

		Quadtree *northWest;
		Quadtree *northEast;
		Quadtree *southWest;
		Quadtree *southEast;




	private:

};

