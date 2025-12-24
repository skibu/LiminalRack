#pragma once
#include <complex>
#include <algorithm> // for std::min, max

#include <common.hpp>


namespace rack {
/** Extends `<cmath>` with extra functions and types */
namespace math {


////////////////////
// basic integer functions
////////////////////

/** Returns true if `x` is odd. */
template <typename T>
bool isEven(T x) {
	return x % 2 == 0;
}

/** Returns true if `x` is odd. */
template <typename T>
bool isOdd(T x) {
	return x % 2 != 0;
}

/** Limits `x` between `min` and `max`.
 * If `max < min`, returns min.
 */
inline int clamp(int x, int min, int max) {
	return std::max(std::min(x, max), min);
}

/** Limits `x` between `min` and `max`.
 * If `max < min`, switches the two values.
 */
inline int clampSafe(int x, int min, int max) {
	return (min <= max) ? clamp(x, min, max) : clamp(x, max, min);
}

/** Euclidean modulus. Always returns `0 <= mod < b`.
`b` must be positive.
See https://en.wikipedia.org/wiki/Euclidean_division
*/
inline int eucMod(int a, int b) {
	int mod = a % b;
	if (mod < 0) {
		mod += b;
	}
	return mod;
}

/** Euclidean division.
`b` must be positive.
*/
inline int eucDiv(int a, int b) {
	int div = a / b;
	int mod = a % b;
	if (mod < 0) {
		div -= 1;
	}
	return div;
}

inline void eucDivMod(int a, int b, int* div, int* mod) {
	*div = a / b;
	*mod = a % b;
	if (*mod < 0) {
		*div -= 1;
		*mod += b;
	}
}

/** Returns `floor(log_2(n))`, or 0 if `n == 1`. */
inline int log2(int n) {
	int i = 0;
	while (n >>= 1) {
		i++;
	}
	return i;
}

/** Returns whether `n` is a power of 2. */
template <typename T>
bool isPow2(T n) {
	return n > 0 && (n & (n - 1)) == 0;
}

/** Returns 1 for positive numbers, -1 for negative numbers, and 0 for zero.
See https://en.wikipedia.org/wiki/Sign_function.
*/
template <typename T>
T sgn(T x) {
	return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

////////////////////
// basic float functions
////////////////////

/** Limits `x` between `min` and `b`.
If `b < min`, returns min.
*/
inline float clamp(float x, float min = 0.f, float max = 1.f) {
	return std::fmax(std::fmin(x, max), min);
}

/** Limits `x` between `min` and `b`.
If `b < min`, switches the two values.
*/
inline float clampSafe(float x, float min = 0.f, float max = 1.f) {
	return (min <= max) ? clamp(x, min, max) : clamp(x, max, min);
}

/** Converts -0.f to 0.f. Leaves all other values unchanged. */
#if defined __clang__
// Clang doesn't support disabling individual optimizations, just everything.
__attribute__((optnone))
#else
__attribute__((optimize("signed-zeros")))
#endif
inline float normalizeZero(float x) {
	return x + 0.f;
}

/** Euclidean modulus. Always returns `0 <= mod < b`.
See https://en.wikipedia.org/wiki/Euclidean_division.
*/
inline float eucMod(float a, float b) {
	float mod = std::fmod(a, b);
	if (mod < 0.f) {
		mod += b;
	}
	return mod;
}

/** Returns whether `a` is within epsilon distance from `b`. */
inline bool isNear(float a, float b, float epsilon = 1e-6f) {
	return std::fabs(a - b) <= epsilon;
}

/** If the magnitude of `x` if less than epsilon, return 0. */
inline float chop(float x, float epsilon = 1e-6f) {
	return std::fabs(x) <= epsilon ? 0.f : x;
}

/** Rescales `x` from the range `[xMin, xMax]` to `[yMin, yMax]`.
*/
inline float rescale(float x, float xMin, float xMax, float yMin, float yMax) {
	return yMin + (x - xMin) / (xMax - xMin) * (yMax - yMin);
}

/** Linearly interpolates between `a` and `b`, from `p = 0` to `p = 1`.
*/
inline float crossfade(float a, float b, float p) {
	return a + (b - a) * p;
}

/** Linearly interpolates an array `p` with index `x`.
The array at `p` must be at least length `floor(x) + 2`.
*/
inline float interpolateLinear(const float* p, float x) {
	int xi = x;
	float xf = x - xi;
	return crossfade(p[xi], p[xi + 1], xf);
}

/** Complex multiplication `c = a * b`.
Arguments may be the same pointers.
Example:

	cmultf(ar, ai, br, bi, &ar, &ai);
*/
inline void complexMult(float ar, float ai, float br, float bi, float* cr, float* ci) {
	*cr = ar * br - ai * bi;
	*ci = ar * bi + ai * br;
}

////////////////////
// 2D vector and rectangle
////////////////////

// Forward dedlaration
class Rect;

/** 2-dimensional vector of floats, representing a point on the plane for graphics.
*/
class Vec {
   private:
    float x_ = 0.f;
    float y_ = 0.f;

   public:
    Vec() {}
    Vec(float xy) : x_(xy), y_(xy) {}
    Vec(float x, float y) : x_(x), y_(y) {}

    /** Sets x, y of the vector. */
    void set(float x, float y) {
        this->x_ = x;
        this->y_ = y;
    }

    /** For when vec is used as position. Simply returns x value as the x
     * position. */
    float getX() const {
        return x_;
    }

    /** Sets the x value of the vector. */
    void setX(float x) {
        this->x_ = x;
    }

    /** For when vec is used as position. Simply returns y value as the y
     * position. */
    float getY() const {
        return y_;
    }

    /** Sets the y value of the vector. */
    void setY(float y) {
        this->y_ = y;
    }

    /** For when vec is used as size. Simply returns x value as the width. */
    float getWidth() const {
        return x_;
    }

    /** Sets the width/x value of the vector. */
    void setWidth(float width) {
        this->x_ = width;
    }

    /** For when vec is used as size. Simply returns y value as the height. */
    float getHeight() const {
        return y_;
    }

    /** Sets the height/y value of the vector. */
    void setHeight(float height) {
        this->y_ = height;
    }

    float& operator[](int i) {
        return (i == 0) ? x_ : y_;
    }
    const float& operator[](int i) const {
        return (i == 0) ? x_ : y_;
    }
    /** Returns a copy of the vector, but negated.
    Equivalent to a reflection across the `y = -x` line.
    */
    Vec neg() const {
        return Vec(-x_, -y_);
    }

    /** Returns copy of this vector with b added */
    Vec plus(Vec b) const {
        return Vec(x_ + b.x_, y_ + b.y_);
    }

    /** Returns copy of this vector with b subtracted */
    Vec minus(Vec b) const {
        return Vec(x_ - b.x_, y_ - b.y_);
    }

    /** Returns copy of this vector scaled by s */
    Vec mult(float s) const {
        return Vec(x_ * s, y_ * s);
    }

    /** Returns copy of this vector multiplied component-wise by b,
     * Vec(x * b.x, y * b.y)
     */
    Vec mult(Vec b) const {
        return Vec(x_ * b.x_, y_ * b.y_);
    }

    /** Returns copy of this vector divided by s, Vec(x / s, y / s) */
    Vec div(float s) const {
        return Vec(x_ / s, y_ / s);
    }

    /** Returns copy of this vector divided component-wise by b,
     * Vec(x / b.x, y / b.y)
     */
    Vec div(Vec b) const {
        return Vec(x_ / b.x_, y_ / b.y_);
    }

    /** Returns the dot product of this vector and b, x * b.x + y * b.y*/
    float dot(Vec b) const {
        return x_ * b.x_ + y_ * b.y_;
    }

    /** Returns the angle of the vector in radians from the positive X axis */
    float arg() const {
        return std::atan2(y_, x_);
    }

    /** Returns the magnitude (length) of the vector */
    float norm() const {
        return std::hypot(x_, y_);
    }

    /** Returns copy of this vector, but normalized (length = 1) */
    Vec normalize() const {
        return div(norm());
    }

    /** Returns the squared magnitude of the vector, x * x + y * y */
    float square() const {
        return x_ * x_ + y_ * y_;
    }

    /** Returns the area represented by the vector, x * y */
    float area() const {
        return x_ * y_;
    }

    /** Rotates counterclockwise in radians. */
    Vec rotate(float angle) {
        float sin = std::sin(angle);
        float cos = std::cos(angle);
        return Vec(x_ * cos - y_ * sin, x_ * sin + y_ * cos);
    }

    /** Swaps the coordinates.
    Equivalent to a reflection across the `y = x` line.
    */
    Vec flip() const {
        return Vec(y_, x_);
    }

    /** Returns minimum of the x, y values of this vector and b. */
    Vec min(Vec b) const {
        return Vec(std::fmin(x_, b.x_), std::fmin(y_, b.y_));
    }

    /** Returns maximum of the x, y values of this vector and b. */
    Vec max(Vec b) const {
        return Vec(std::fmax(x_, b.x_), std::fmax(y_, b.y_));
    }

    /** Returns copy of this vector with each component replaced by its absolute
     * value. */
    Vec abs() const {
        return Vec(std::fabs(x_), std::fabs(y_));
    }

    /** Returns copy of this vector with each component rounded to the nearest
     * integer. */
    Vec round() const {
        return Vec(std::round(x_), std::round(y_));
    }

    /** Returns copy of this vector with each component rounded down to the
     * nearest integer. */
    Vec floor() const {
        return Vec(std::floor(x_), std::floor(y_));
    }

    /** Returns copy of this vector with each component rounded up to the
     * nearest integer. */
    Vec ceil() const {
        return Vec(std::ceil(x_), std::ceil(y_));
    }

    /** Returns whether this vector is equal to vector b. */
    bool equals(Vec b) const {
        return x_ == b.x_ && y_ == b.y_;
    }

    /** Alias for equals() */
    bool isEqual(Vec b) const {
        return equals(b);
    }

    /** Returns whether this vector is the zero vector (0, 0). */
    bool isZero() const {
        return x_ == 0.f && y_ == 0.f;
    }

    /** Returns whether both components are finite (not infinite or NaN). */
    bool isFinite() const {
        return std::isfinite(x_) && std::isfinite(y_);
    }

    /** Returns copy of this vector, clamped to the given bounds. */
    Vec clamp(Rect bound) const;

    /** Returns copy of this vector, safe clamped to the given bounds. */
    Vec clampSafe(Rect bound) const;

    /** Linearly interpolates between this vector and b, from p = 0 to p = 1. */
    Vec crossfade(Vec b, float p) {
        return this->plus(b.minus(*this).mult(p));
    }
};

/** 2-dimensional rectangle for graphics.
Mathematically, Rects include points on its left/top edge but *not* its right/bottom edge.
The infinite Rect (equal to the entire plane) is defined using pos=-inf and size=inf.
*/
class Rect {
   private:
    Vec pos_;
    Vec size_;

   public:
    Rect() {}
    Rect(Vec pos, Vec size) : pos_(pos), size_(size) {}
    Rect(float posX, float posY, float sizeX, float sizeY)
        : pos_(Vec(posX, posY)), size_(Vec(sizeX, sizeY)) {}

    /** Sets the x, y position of the rectangle to values of the pos parameter
     */
    void setPos(const Vec& pos) {
        this->pos_ = pos;
    }

    /** Sets the x, y position of the rectangle to values of the rect parameter */
    void setPos(float x, float y) {
        this->pos_ = Vec(x, y);
    }

    /** Sets x, y position of the rectangle to values of the rect parameter */
    void setPos(const Rect& rect) {
        this->pos_ = rect.pos_;
    }

    /** Returns the x, y position of the rectangle. */
    Vec getPos() const {
        return pos_;
    }

    /** Sets the x position of the rectangle. */
    void setPosX(float x) {
        pos_.setX(x);
    }   

    /** Sets the y position of the rectangle. */
    void setPosY(float y) {
        pos_.setY(y);
    }

    /** Returns the x position of the rectangle. */
    float getPosX() const {
        return pos_.getX();
    }

    /** Returns the y position of the rectangle. */
    float getPosY() const {
        return pos_.getY();
    }

       /** Sets the x position of the rectangle. */
    void setX(float x) {
        pos_.setX(x);
    }   

    /** Sets the y position of the rectangle. */
    void setY(float y) {
        pos_.setY(y);
    }

    /** Returns the x position of the rectangle. */
    float getX() const {
        return getPosX();
    }

    /** Returns the y position of the rectangle. */
    float getY() const {
        return getPosY();
    }

    /** Sets the width & height size of the rectangle to values of the size parameter */
    void setSize(const Vec& size) {
        this->size_ = size;
    }

    /** Sets the width & height size of the rectangle to values of the rect parameter */
    void setSize(float width, float height) {
        this->size_ = Vec(width, height);
    }

    /** Sets the width & height size of the rectangle to values of the rect parameter */
    void setSize(const Rect& rect) {
        this->size_ = rect.size_;
    }

    /** Returns the width & height size of the rectangle. */
    Vec getSize() const {
        return size_;
    }

    /** Returns the width size of the rectangle. */
    float getSizeX() const {
        return size_.getWidth();
    }

    /** Returns the height size of the rectangle. */
    float getSizeY() const {
        return size_.getHeight();
    }

    /** Returns the width size of the rectangle. */
    float getWidth() const {
        return size_.getWidth();
    }

    /** Sets the width size of the rectangle. */
    void setWidth(float width) {
        size_.setWidth(width);
    }

    /** Returns the height size of the rectangle. */
    float getHeight() const {
        return size_.getHeight();
    }

    /** Sets the height size of the rectangle. */
    void setHeight(float height) {
        size_.setHeight(height);
    }

	/** Constructs a Rect from a top-left and bottom-right vector. */
	static Rect fromMinMax(Vec a, Vec b) {
		return Rect(a, b.minus(a));
	}

	/** Constructs a Rect from any two opposite corners. */
	static Rect fromCorners(Vec a, Vec b) {
		return fromMinMax(a.min(b), a.max(b));
	}

	/** Returns Rect where pos=-inf and size=inf */
	static Rect inf() {
		return Rect(Vec(-INFINITY, -INFINITY), Vec(INFINITY, INFINITY));
	}

	/** Returns whether this Rect contains a point, inclusive on the left/top, exclusive on the right/bottom.
	Correctly handles infinite Rects.
	*/
	bool contains(Vec v) const {
		return (pos_.getX() <= v.getX()) && (size_.getX() == INFINITY || v.getX() < pos_.getX() + size_.getX())
		    && (pos_.getY() <= v.getY()) && (size_.getY() == INFINITY || v.getY() < pos_.getY() + size_.getY());
	}

	/** Returns whether this Rect contains (is a superset of) a Rect.
	Correctly handles infinite Rects.
	*/
	bool contains(Rect r) const {
		return (pos_.getX() <= r.pos_.getX()) && (r.pos_.getX() - size_.getX() <= pos_.getX() - r.size_.getX())
		    && (pos_.getY() <= r.pos_.getY()) && (r.pos_.getY() - size_.getY() <= pos_.getY() - r.size_.getY());
	}

    /** Returns whether this Rect overlaps with another Rect.
    Correctly handles infinite Rects.
    */
    bool intersects(Rect r) const {
        return (r.getWidth() == INFINITY ||
                pos_.getX() < r.pos_.getX() + r.getWidth()) &&
                (size_.getWidth() == INFINITY ||
                r.pos_.getX() < pos_.getX() + size_.getWidth()) &&
                (r.getHeight() == INFINITY ||
                pos_.getY() < r.pos_.getY() + r.getHeight()) &&
                (size_.getHeight() == INFINITY ||
                r.pos_.getY() < pos_.getY() + size_.getHeight());
    }

    /** Returns whether this Rect is equal to another Rect. */
	bool equals(Rect r) const {
		return pos_.equals(r.pos_) && size_.equals(r.size_);
	}

    /** Returns the x position of the rectangle, pos.x. */
	float getLeft() const {
		return pos_.getX();
	}

    /** Returns the position of the right side of the rectangle, pos.x +
     * size.x. */
    float getRight() const {
        return (size_.getWidth() == INFINITY)
                    ? INFINITY
                    : (pos_.getX() + size_.getWidth());
    }

    /** Returns the position of the top side of the rectangle, pos.y. */
    float getTop() const {
        return pos_.getY();
    }

    /** Returns the position of the bottom side of the rectangle, pos.y +
     * size.y. */
    float getBottom() const {
        return (size_.getHeight() == INFINITY)
                    ? INFINITY
                    : (pos_.getY() + size_.getHeight());
    }

    /** Returns the center point of the rectangle.
	Returns a NaN coordinate if pos=-inf and size=inf.
	*/
	Vec getCenter() const {
		return pos_.plus(size_.mult(0.5f));
	}

    /** Returns the x, y position of the top-left corner of the rectangle. */
	Vec getTopLeft() const {
		return pos_;
	}

    /** Returns the x, y position of the top-right corner of the rectangle. */
	Vec getTopRight() const {
		return Vec(getRight(), getTop());
	}

    /** Returns the x, y position of the bottom-left corner of the rectangle. */
	Vec getBottomLeft() const {
		return Vec(getLeft(), getBottom());
	}

    /** Returns the x, y position of the bottom-right corner of the rectangle. */
	Vec getBottomRight() const {
		return Vec(getRight(), getBottom());
    }

    /** Clamps the edges of the rectangle to fit within a bound. */
    Rect clamp(Rect bound) const {
        Rect r;
        r.setPos(math::clampSafe(getX(), bound.getX(),
                                 bound.getX() + bound.getWidth()),
                 math::clampSafe(getY(), bound.getY(),
                                 bound.getY() + bound.getHeight()));

        r.setSize(math::clamp(getX() + getWidth(), bound.getX(),
                              bound.getX() + bound.getWidth()) -
                      r.getX(),
                  math::clamp(getY() + size_.getHeight(), bound.getY(),
                              bound.getY() + bound.getHeight()) -
                      r.getY());
        return r;
    }

    /** Updates the x, y position so rectangle fits inside the bound box
     * parameter. */
    Rect nudge(Rect bound) const {
        Rect r;
        r.size_ = size_;
        r.setPos(
            math::clampSafe(pos_.getX(), bound.getX(),
                            bound.getX() + bound.getWidth() - size_.getWidth()),
            math::clampSafe(
                pos_.getY(), bound.getY(),
                bound.getY() + bound.getHeight() - size_.getHeight()));
        return r;
    }

    /** Returns the bounding box of the union of `this` rectangle and the `b`
     * rectangle. */
    Rect expand(Rect b) const {
        Rect r;
        r.setPos(std::fmin(pos_.getX(), b.pos_.getX()),
                 std::fmin(pos_.getY(), b.pos_.getY()));
        r.setSize(std::fmax(pos_.getX() + size_.getWidth(), b.pos_.getX() + b.size_.getWidth()) - r.getX(),
                  std::fmax(pos_.getY() + size_.getHeight(), b.pos_.getY() + b.size_.getHeight()) - r.getY());
        return r;
    }

    /** Returns the intersection of `this` rectangle and the `b` rectangle. */
    Rect intersect(Rect b) const {
        Rect r;
        r.setPos(std::fmax(pos_.getX(), b.pos_.getX()),
                 std::fmax(pos_.getY(), b.pos_.getY()));
        r.setSize(std::fmin(pos_.getX() + size_.getWidth(),
                            b.pos_.getX() + b.size_.getWidth()) -
                      r.getX(),
                  std::fmin(pos_.getY() + size_.getHeight(),
                            b.pos_.getY() + b.size_.getHeight()) -
                      r.getY());
        return r;
    }

    /** Returns this Rect but with its position set to zero. */
	Rect zeroPos() const {
		return Rect(Vec(), size_);
	}

    /** Returns copy of this rectangle, adding deltaSize to size */
    Rect addSize(Vec deltaSize) const {
        // Use copy of this rectangle
        Rect r = *this;

        // Add deltaSize to size
        r.size_ = size_.plus(deltaSize);

        return r;
    }

    /** Returns copy of this rectangle, expanding each corner by the delta
     * parameter. */
    Rect grow(Vec delta) const {
        Rect r;
        r.pos_ = pos_.minus(delta);
        r.size_ = size_.plus(delta.mult(2.f));
        return r;
    }

    /** Returns copy of this rectangle, contracted each corner by the delta
     * parameter. */
    Rect shrink(Vec delta) const {
        Rect r;
        r.pos_ = pos_.plus(delta);
        r.size_ = size_.minus(delta.mult(2.f));
        return r;
	}
    
	/** Returns `pos + size * p` */
	Vec interpolate(Vec p) {
		return pos_.plus(size_.mult(p));
	}

	// Method aliases

    /** Alias for contains() */
	bool isContaining(Vec v) const {
		return contains(v);
	}

    /** Alias for intersects() */
	bool isIntersecting(Rect r) const {
		return intersects(r);
	}
    
    /** Alias for equals() */
	bool isEqual(Rect r) const {
		return equals(r);
	}
};


inline Vec Vec::clamp(Rect bound) const {
	return Vec(
		math::clamp(x_, bound.getPosX(), bound.getPosX() + bound.getSizeX()),
		math::clamp(y_, bound.getPosY(), bound.getPosY() + bound.getSizeY())
	);
}

inline Vec Vec::clampSafe(Rect bound) const {
	return Vec(
		math::clampSafe(x_, bound.getPosX(), bound.getPosX() + bound.getSizeX()),
		math::clampSafe(y_, bound.getPosY(), bound.getPosY() + bound.getSizeY())
	);
}


// Operator overloads for Vec
inline Vec operator+(const Vec& a) {
	return a;
}
inline Vec operator-(const Vec& a) {
	return a.neg();
}
inline Vec operator+(const Vec& a, const Vec& b) {
	return a.plus(b);
}
inline Vec operator-(const Vec& a, const Vec& b) {
	return a.minus(b);
}
inline Vec operator*(const Vec& a, const Vec& b) {
	return a.mult(b);
}
inline Vec operator*(const Vec& a, const float& b) {
	return a.mult(b);
}
inline Vec operator*(const float& a, const Vec& b) {
	return b.mult(a);
}
inline Vec operator/(const Vec& a, const Vec& b) {
	return a.div(b);
}
inline Vec operator/(const Vec& a, const float& b) {
	return a.div(b);
}
inline Vec operator+=(Vec& a, const Vec& b) {
	return a = a.plus(b);
}
inline Vec operator-=(Vec& a, const Vec& b) {
	return a = a.minus(b);
}
inline Vec operator*=(Vec& a, const Vec& b) {
	return a = a.mult(b);
}
inline Vec operator*=(Vec& a, const float& b) {
	return a = a.mult(b);
}
inline Vec operator/=(Vec& a, const Vec& b) {
	return a = a.div(b);
}
inline Vec operator/=(Vec& a, const float& b) {
	return a = a.div(b);
}
inline bool operator==(const Vec& a, const Vec& b) {
	return a.equals(b);
}
inline bool operator!=(const Vec& a, const Vec& b) {
	return !a.equals(b);
}


// Operator overloads for Rect
inline bool operator==(const Rect& a, const Rect& b) {
	return a.equals(b);
}
inline bool operator!=(const Rect& a, const Rect& b) {
	return !a.equals(b);
}


/** Expands a Vec and Rect into a comma-separated list.
Useful for print debugging.

	printf("(%f %f) (%f %f %f %f)", VEC_ARGS(v), RECT_ARGS(r));

Or passing the values to a C function.

	nvgRect(vg, RECT_ARGS(r));
*/
#define VEC_ARGS(v) (v).getX(), (v).getY()
#define RECT_ARGS(r) (r).getX(), (r).getY(), (r).getWidth(), (r).getHeight()


} // namespace math
} // namespace rack
