#pragma once
#include "hittable.h"
#include <vector>
#include "hittable_list.h"
#include <algorithm>
#include "random.h"

class BVHNode : public Hittable
{
    public:
        std::shared_ptr<Hittable> left, right;
        AABB bbox;

        bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override
        {
            if (!(bbox.hit(r, tmin, tmax)))
            {
                return false;
            }

            bool hitLeft = left -> hit(r, tmin, tmax, rec);
            bool hitRight = right ->hit(r, tmin, hitLeft ? rec.t : tmax, rec);

            return (hitLeft || hitRight);

        }

        static bool boxXCompare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b)
        {
            return a->boundingBox().min.x < b->boundingBox().min.x;
        }

         static bool boxYCompare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b)
        {
            return a->boundingBox().min.y < b->boundingBox().min.y;
        }
         static bool boxZCompare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b)
        {
            return a->boundingBox().min.z < b->boundingBox().min.z;
        }

        AABB boundingBox() const override
        {
            return bbox;
        }

        BVHNode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end)
        {
            size_t span = end - start;
            if (span == 1)
            {
                left = right = objects[start];
            }
            else if (span == 2)
            {
                left = objects[start];
                right = objects[start + 1];
            }

            else
            {
                int randomAxis = int(randomDouble() * 3);
                auto comparator =
                (randomAxis == 0) ? boxXCompare
                : (randomAxis == 1) ? boxYCompare
                : boxZCompare;

                std::sort(objects.begin() + start, objects.begin() + end, comparator);
                size_t mid = start + span / 2;
                left  = std::make_shared<BVHNode>(objects, start, mid);
                right = std::make_shared<BVHNode>(objects, mid, end);
            }

            bbox = AABB(left->boundingBox(), right->boundingBox());
        }

        BVHNode(HittableList list) : BVHNode(list.objects, 0, list.objects.size()) {}

};