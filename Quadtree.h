#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

template <typename T>
class Quadtree
{
	public:
		Quadtree(glm::vec2 centre, glm::vec2 size);
		~Quadtree();

		bool insert(T obj, glm::vec2 p);
		bool inBoundry(glm::vec2 p);
		bool inBoundry(glm::vec2 p, glm::vec2 c, glm::vec2 s);
		bool boundryIntersect(glm::vec2 c1, glm::vec2 s1, glm::vec2 c2, glm::vec2 s2);
		bool search(glm::vec2 p);
		
		std::vector<T> search(glm::vec2 c, glm::vec2 s);

		glm::vec2 getCentre();
		glm::vec2 getSize();

		void subdivide();

	protected:


		bool leaf;

		glm::vec2 centre;
		glm::vec2 size;

		std::vector<T> objects;
		std::vector<glm::vec2> points;

		Quadtree *northWest;
		Quadtree *northEast;
		Quadtree *southWest;
		Quadtree *southEast;




	private:

};



template <typename T>
Quadtree<T>::Quadtree(glm::vec2 c, glm::vec2 s)
{
	centre = c;
	size = s;
	leaf = true;
}

template <typename T>
bool Quadtree<T>::insert(T obj, glm::vec2 p)
{
	if (!leaf)
	{
		if (northWest->inBoundry(p))
		{
			return northWest->insert(obj, p);
		}
		else if (northEast->inBoundry(p))
		{
			return northEast->insert(obj, p);
		}
		else if (southWest->inBoundry(p))
		{
			return southWest->insert(obj, p);
		}
		else if (southEast->inBoundry(p))
		{
			return southEast->insert(obj, p);
		}

		return false;
	}

	if (points.size() < 4 && inBoundry(p))
	{
		points.push_back(p);
		objects.push_back(obj);
		return true;
	}

	subdivide();

	return insert(obj, p);
}

template <typename T>
bool Quadtree<T>::inBoundry(glm::vec2 p)
{
	return inBoundry(p, centre, size);
}

template <typename T>
bool Quadtree<T>::inBoundry(glm::vec2 p, glm::vec2 c, glm::vec2 s)
{
	float startLength = c.x - s.x / 2;
	float endLength = c.x + s.x / 2;

	float startHeight = c.y - s.y / 2;
	float endHeight = c.y + s.y / 2;

	if (p.x <= endLength && p.x >= startLength &&
		p.y <= endHeight && p.y >= startHeight)
	{
		return true;
	}

	return false;
}

template <typename T>
bool Quadtree<T>::boundryIntersect(glm::vec2 c1, glm::vec2 s1, glm::vec2 c2, glm::vec2 s2)
{
	glm::vec2 topLeftCorner = glm::vec2(c1.x - 0.5*s1.x, c1.y + 0.5*s1.y);
	glm::vec2 topRightCorner = glm::vec2(c1.x + 0.5*s1.x, c1.y + 0.5*s1.y);
	glm::vec2 botLeftCorner = glm::vec2(c1.x - 0.5*s1.x, c1.y - 0.5*s1.y);
	glm::vec2 botRightCorner = glm::vec2(c1.x + 0.5*s1.x, c1.y - 0.5*s1.y);

	if (inBoundry(topLeftCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(topRightCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(botLeftCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(botRightCorner, c2, s2))
	{
		return true;
	}

	return false;
}

template <typename T>
bool Quadtree<T>::search(glm::vec2 p)
{
	if (leaf)
	{
		for (int i = 0; i < points.size(); i++)
		{
			if (p == points[i])
			{
				return true;
			}
		}

		return false;
	}

	if (northWest->inBoundry(p))
	{
		return northWest->search(p);
	}
	else if (northEast->inBoundry(p))
	{
		return northEast->search(p);
	}
	else if (southWest->inBoundry(p))
	{
		return southWest->search(p);
	}
	else if (southEast->inBoundry(p))
	{
		return southEast->search(p);
	}

	return false;
}

template <typename T>
std::vector<T> Quadtree<T>::search(glm::vec2 c, glm::vec2 s)
{
	std::vector<T> results;
	if (leaf)
	{
		for (int i = 0; i < points.size(); i++)
		{
			if (inBoundry(points[i], c, s))
			{
				results.push_back(objects[i]);
			}
		}
	}
	else
	{
		std::vector<T> childResult;

		if (boundryIntersect(northWest->getCentre(), northWest->getSize(), c, s))
		{
			childResult = northWest->search(c, s);
			for (int i = 0; i < childResult.size(); i++)
			{
				results.push_back(childResult[i]);
			}
		}

		if (boundryIntersect(northEast->getCentre(), northEast->getSize(), c, s))
		{
			childResult = northEast->search(c, s);
			for (int i = 0; i < childResult.size(); i++)
			{
				results.push_back(childResult[i]);
			}
		}

		if (boundryIntersect(southWest->getCentre(), southWest->getSize(), c, s))
		{
			childResult = southWest->search(c, s);
			for (int i = 0; i < childResult.size(); i++)
			{
				results.push_back(childResult[i]);
			}
		}

		if (boundryIntersect(southEast->getCentre(), southEast->getSize(), c, s))
		{
			childResult = southEast->search(c, s);
			for (int i = 0; i < childResult.size(); i++)
			{
				results.push_back(childResult[i]);
			}
		}
	}

	return results;
}

template <typename T>
glm::vec2 Quadtree<T>::getCentre()
{
	return centre;
}

template <typename T>
glm::vec2 Quadtree<T>::getSize()
{
	return size;
}

template <typename T>
void Quadtree<T>::subdivide()
{
	leaf = false;

	glm::vec2 newCentre = glm::vec2(centre.x - 0.25 * size.x, centre.y + 0.25 * size.y);
	glm::vec2 newSize = glm::vec2(size.x / 2, size.y / 2);
	northWest = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x + 0.25 * size.x, centre.y + 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
	northEast = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x - 0.25 * size.x, centre.y - 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
	southWest = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x + 0.25 * size.x, centre.y - 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
	southEast = new Quadtree(newCentre, newSize);

	for (int i = 0; i < points.size(); i++)
	{
		insert(objects[i], points[i]);
	}

	points.clear();
	objects.clear();
}

template <typename T>
Quadtree<T>::~Quadtree()
{
}

