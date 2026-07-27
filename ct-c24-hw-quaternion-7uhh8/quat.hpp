#ifndef QUAT_HPP
#define QUAT_HPP

#include <cmath>

template< typename T >
struct matrix_t
{
	T data[16];
};

template< typename T >
struct vector3_t
{
	T x, y, z;
};

template< typename T >
class Quat
{
  public:
	Quat() : m_value{ 0, 0, 0, 0 } {}

	Quat(T a, T b, T c, T d) : m_value{ b, c, d, a } {}

	Quat(T phi, bool radians, vector3_t< T > rotation_vect)
	{
		T x = rotation_vect.x, y = rotation_vect.y, z = rotation_vect.z;
		T norm = std::sqrt(x * x + y * y + z * z);
		T half_phi = static_cast< T >(radians ? phi / 2 : phi * M_PI / 360);

		m_value[0] = (x / norm) * std::sin(half_phi);
		m_value[1] = (y / norm) * std::sin(half_phi);
		m_value[2] = (z / norm) * std::sin(half_phi);
		m_value[3] = std::cos(half_phi);
	}

	// operators
	explicit operator T() const
	{
		return std::sqrt(m_value[0] * m_value[0] + m_value[1] * m_value[1] + m_value[2] * m_value[2] + m_value[3] * m_value[3]);
	}

	Quat< T > operator+(const Quat< T > &quat) const
	{
		return Quat< T >(m_value[3] + quat.m_value[3], m_value[0] + quat.m_value[0], m_value[1] + quat.m_value[1], m_value[2] + quat.m_value[2]);
	}

	Quat< T > &operator+=(const Quat< T > &quat)
	{
		*this = *this + quat;
		return *this;
	}

	Quat< T > operator-(const Quat< T > &quat) const
	{
		return Quat< T >(m_value[3] - quat.m_value[3], m_value[0] - quat.m_value[0], m_value[1] - quat.m_value[1], m_value[2] - quat.m_value[2]);
	}

	Quat< T > &operator-=(const Quat< T > &quat)
	{
		*this = *this - quat;
		return *this;
	}

	Quat< T > operator*(const Quat< T > &quat) const
	{
		T s_a = m_value[3];
		T s_b = quat.m_value[3];
		T x_a = m_value[0], y_a = m_value[1], z_a = m_value[2];
		T x_b = quat.m_value[0], y_b = quat.m_value[1], z_b = quat.m_value[2];

		T s = s_a * s_b - x_a * x_b - y_a * y_b - z_a * z_b;
		T x = s_a * x_b + s_b * x_a + y_a * z_b - y_b * z_a;
		T y = s_a * y_b + s_b * y_a + z_a * x_b - z_b * x_a;
		T z = s_a * z_b + s_b * z_a + x_a * y_b - x_b * y_a;

		return Quat< T >(s, x, y, z);
	}

	Quat< T > operator*(const T scalar) const
	{
		return Quat< T >(scalar * m_value[3], scalar * m_value[0], scalar * m_value[1], scalar * m_value[2]);
	}

	Quat< T > operator*(const vector3_t< T > &vect) const { return *this * Quat< T >(0, vect.x, vect.y, vect.z); }

	Quat< T > operator~() const { return Quat< T >(m_value[3], -m_value[0], -m_value[1], -m_value[2]); }

	bool operator==(const Quat< T > &quat) const
	{
		return m_value[0] == quat.m_value[0] && m_value[1] == quat.m_value[1] && m_value[2] == quat.m_value[2] &&
			   m_value[3] == quat.m_value[3];
	}

	bool operator!=(const Quat< T > &quat) const { return !(*this == quat); }

	// methods

	const T *data() const { return m_value; };

	T angle(bool radians = true) const
	{
		T angle_in_radians = 2 * std::acos(m_value[3]);
		return radians ? angle_in_radians : angle_in_radians * 180 / M_PI;
	}

	matrix_t< T > matrix() const
	{
		T a = m_value[3], b = m_value[0], c = m_value[1], d = m_value[2];

		return matrix_t< T >({ a, -b, -c, -d, b, a, -d, c, c, d, a, -b, d, -c, b, a });
	}

	matrix_t< T > rotation_matrix() const
	{
		T norm = T(*this);

		T x = m_value[0] / norm;
		T y = m_value[1] / norm;
		T z = m_value[2] / norm;
		T w = m_value[3] / norm;

		T xx = x * x, yy = y * y, zz = z * z;
		T xy = x * y, xz = x * z, yz = y * z;
		T wx = w * x, wy = w * y, wz = w * z;

		return matrix_t< T >(
			{ 1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0, 2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0, 2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0, 0, 0, 0, 1 });
	}

	vector3_t< T > apply(vector3_t< T > vect) const
	{
		T norm = T(*this);
		Quat< T > result = (*this * (1 / norm)) * vect;
		result = result * ~(*this * (1 / norm));

		return { result.m_value[0], result.m_value[1], result.m_value[2] };
	}

  private:
	T m_value[4];	 // in sequence [b, c, d, a]
};

#endif
