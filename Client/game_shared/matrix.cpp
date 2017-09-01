//=======================================================================
//			Copyright (C) XashXT Group 2011
//=======================================================================

#include "..\game_shared\mathlib.h"
#include "..\game_shared\matrix.h"
#include "..\common\const.h"
#include "..\common\com_model.h"

matrix3x3::matrix3x3( void )
{
}

void matrix3x3::Identity( void )
{
	mat[0] = Vector( 1, 0, 0 );
	mat[1] = Vector( 0, 1, 0 );
	mat[2] = Vector( 0, 0, 1 );
}

void matrix3x3 :: FromVector( const Vector &forward )
{
	if( forward.x || forward.y )
	{
		mat[0] = forward;
		mat[1] = Vector( forward.y, -forward.x, 0.0f ).Normalize();
		mat[2] = CrossProduct( mat[0], mat[1] );
	}
	else
	{
		mat[0] = forward;
		mat[1] = Vector( 1.0f, 0.0f, 0.0f );
		mat[2] = Vector( 0.0f, 1.0f, 0.0f );
	}
}

// from class matrix3x4 to class matrix3x3
matrix3x3& matrix3x3 :: operator=(const matrix3x4 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][1] = vOther[1][1];
	mat[2][2] = vOther[2][2];
	mat[0][1] = vOther[1][0];
	mat[0][2] = vOther[1][0];
	mat[1][0] = vOther[0][1];
	mat[1][2] = vOther[2][1];
	mat[2][0] = vOther[0][2];
	mat[2][1] = vOther[1][2];

	return *this;
}

// from class matrix4x4 to class matrix3x3
matrix3x3& matrix3x3 :: operator=(const matrix4x4 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][1] = vOther[1][1];
	mat[2][2] = vOther[2][2];
	mat[0][1] = vOther[1][0];
	mat[0][2] = vOther[1][0];
	mat[1][0] = vOther[0][1];
	mat[1][2] = vOther[2][1];
	mat[2][0] = vOther[0][2];
	mat[2][1] = vOther[1][2];
	return *this;
}

Vector matrix3x3::VectorRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[1][0] + v[2] * mat[2][0];
	out[1] = v[0] * mat[0][1] + v[1] * mat[1][1] + v[2] * mat[2][1];
	out[2] = v[0] * mat[0][2] + v[1] * mat[1][2] + v[2] * mat[2][2];

	return out;
}

Vector matrix3x3::VectorIRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[0][1] + v[2] * mat[0][2];
	out[1] = v[0] * mat[1][0] + v[1] * mat[1][1] + v[2] * mat[1][2];
	out[2] = v[0] * mat[2][0] + v[1] * mat[2][1] + v[2] * mat[2][2];

	return out;
}

matrix3x3 matrix3x3 :: Concat( const matrix3x3 mat2 )
{
	matrix3x3 out;

	out[0][0] = mat[0][0] * mat2[0][0] + mat[0][1] * mat2[1][0] + mat[0][2] * mat2[2][0];
	out[0][1] = mat[0][0] * mat2[0][1] + mat[0][1] * mat2[1][1] + mat[0][2] * mat2[2][1];
	out[0][2] = mat[0][0] * mat2[0][2] + mat[0][1] * mat2[1][2] + mat[0][2] * mat2[2][2];
	out[1][0] = mat[1][0] * mat2[0][0] + mat[1][1] * mat2[1][0] + mat[1][2] * mat2[2][0];
	out[1][1] = mat[1][0] * mat2[0][1] + mat[1][1] * mat2[1][1] + mat[1][2] * mat2[2][1];
	out[1][2] = mat[1][0] * mat2[0][2] + mat[1][1] * mat2[1][2] + mat[1][2] * mat2[2][2];
	out[2][0] = mat[2][0] * mat2[0][0] + mat[2][1] * mat2[1][0] + mat[2][2] * mat2[2][0];
	out[2][1] = mat[2][0] * mat2[0][1] + mat[2][1] * mat2[1][1] + mat[2][2] * mat2[2][1];
	out[2][2] = mat[2][0] * mat2[0][2] + mat[2][1] * mat2[1][2] + mat[2][2] * mat2[2][2];

	return out;
}

matrix3x4::matrix3x4( void )
{
}

void matrix3x4::Identity( void )
{
	mat[0] = Vector( 1, 0, 0 );
	mat[1] = Vector( 0, 1, 0 );
	mat[2] = Vector( 0, 0, 1 );
	mat[3] = Vector( 0, 0, 0 );
}

Vector matrix3x4::VectorTransform( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[1][0] + v[2] * mat[2][0] + mat[3][0];
	out[1] = v[0] * mat[0][1] + v[1] * mat[1][1] + v[2] * mat[2][1] + mat[3][1];
	out[2] = v[0] * mat[0][2] + v[1] * mat[1][2] + v[2] * mat[2][2] + mat[3][2];

	return out;
}

Vector matrix3x4::VectorITransform( const Vector &v ) const
{
	Vector iv, out;

	iv[0] = v[0] - mat[3][0];
	iv[1] = v[1] - mat[3][1];
	iv[2] = v[2] - mat[3][2];

	out[0] = iv[0] * mat[0][0] + iv[1] * mat[0][1] + iv[2] * mat[0][2];
	out[1] = iv[0] * mat[1][0] + iv[1] * mat[1][1] + iv[2] * mat[1][2];
	out[2] = iv[0] * mat[2][0] + iv[1] * mat[2][1] + iv[2] * mat[2][2];

	return out;
}

Vector matrix3x4::VectorRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[1][0] + v[2] * mat[2][0];
	out[1] = v[0] * mat[0][1] + v[1] * mat[1][1] + v[2] * mat[2][1];
	out[2] = v[0] * mat[0][2] + v[1] * mat[1][2] + v[2] * mat[2][2];

	return out;
}

Vector matrix3x4::VectorIRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[0][1] + v[2] * mat[0][2];
	out[1] = v[0] * mat[1][0] + v[1] * mat[1][1] + v[2] * mat[1][2];
	out[2] = v[0] * mat[2][0] + v[1] * mat[2][1] + v[2] * mat[2][2];

	return out;
}

matrix3x4 matrix3x4 :: Invert( void ) const
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
	float scale = 1.0 / (mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);

	matrix3x4 out;

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out[0][0] = mat[0][0] * scale;
	out[0][1] = mat[1][0] * scale;
	out[0][2] = mat[2][0] * scale;
	out[1][0] = mat[0][1] * scale;
	out[1][1] = mat[1][1] * scale;
	out[1][2] = mat[2][1] * scale;
	out[2][0] = mat[0][2] * scale;
	out[2][1] = mat[1][2] * scale;
	out[2][2] = mat[2][2] * scale;

	// invert the translate
	out[3][0] = -(mat[3][0] * out[0][0] + mat[3][1] * out[1][0] + mat[3][2] * out[2][0]);
	out[3][1] = -(mat[3][0] * out[0][1] + mat[3][1] * out[1][1] + mat[3][2] * out[2][1]);
	out[3][2] = -(mat[3][0] * out[0][2] + mat[3][1] * out[1][2] + mat[3][2] * out[2][2]);

	return out;
}

matrix3x4 matrix3x4 :: ConcatTransforms( const matrix3x4 mat2 )
{
	matrix3x4 out;

	out[0][0] = mat[0][0] * mat2[0][0] + mat[1][0] * mat2[0][1] + mat[2][0] * mat2[0][2];
	out[1][0] = mat[0][0] * mat2[1][0] + mat[1][0] * mat2[1][1] + mat[2][0] * mat2[1][2];
	out[2][0] = mat[0][0] * mat2[2][0] + mat[1][0] * mat2[2][1] + mat[2][0] * mat2[2][2];
	out[3][0] = mat[0][0] * mat2[3][0] + mat[1][0] * mat2[3][1] + mat[2][0] * mat2[3][2] + mat[3][0];
	out[0][1] = mat[0][1] * mat2[0][0] + mat[1][1] * mat2[0][1] + mat[2][1] * mat2[0][2];
	out[1][1] = mat[0][1] * mat2[1][0] + mat[1][1] * mat2[1][1] + mat[2][1] * mat2[1][2];
	out[2][1] = mat[0][1] * mat2[2][0] + mat[1][1] * mat2[2][1] + mat[2][1] * mat2[2][2];
	out[3][1] = mat[0][1] * mat2[3][0] + mat[1][1] * mat2[3][1] + mat[2][1] * mat2[3][2] + mat[3][1];
	out[0][2] = mat[0][2] * mat2[0][0] + mat[1][2] * mat2[0][1] + mat[2][2] * mat2[0][2];
	out[1][2] = mat[0][2] * mat2[1][0] + mat[1][2] * mat2[1][1] + mat[2][2] * mat2[1][2];
	out[2][2] = mat[0][2] * mat2[2][0] + mat[1][2] * mat2[2][1] + mat[2][2] * mat2[2][2];
	out[3][2] = mat[0][2] * mat2[3][0] + mat[1][2] * mat2[3][1] + mat[2][2] * mat2[3][2] + mat[3][2];

	return out;
}

// from class matrix3x3 to class matrix3x4
matrix3x4& matrix3x4 :: operator=(const matrix3x3 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][0] = vOther[1][0];
	mat[2][0] = vOther[2][0];
	mat[3][0] = 0.0f;
	mat[0][1] = vOther[0][1];
	mat[1][1] = vOther[1][1];
	mat[2][1] = vOther[2][1];
	mat[3][1] = 0.0f;
	mat[0][2] = vOther[0][2];
	mat[1][2] = vOther[1][2];
	mat[2][2] = vOther[2][2];
	mat[3][2] = 0.0f;

	return *this;
}

// from class matrix4x4 to class matrix3x4
matrix3x4& matrix3x4 :: operator=(const matrix4x4 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][0] = vOther[1][0];
	mat[2][0] = vOther[2][0];
	mat[3][0] = vOther[3][0];
	mat[0][1] = vOther[0][1];
	mat[1][1] = vOther[1][1];
	mat[2][1] = vOther[2][1];
	mat[3][1] = vOther[3][1];
	mat[0][2] = vOther[0][2];
	mat[1][2] = vOther[1][2];
	mat[2][2] = vOther[2][2];
	mat[3][2] = vOther[3][2];

	return *this;
}

matrix4x4::matrix4x4( void )
{
}

void matrix4x4::Identity( void )
{
	mat[0] = Vector4D( 1, 0, 0, 0 );
	mat[1] = Vector4D( 0, 1, 0, 0 );
	mat[2] = Vector4D( 0, 0, 1, 0 );
	mat[3] = Vector4D( 0, 0, 0, 1 );
}

Vector matrix4x4::VectorTransform( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[1][0] + v[2] * mat[2][0] + mat[3][0];
	out[1] = v[0] * mat[0][1] + v[1] * mat[1][1] + v[2] * mat[2][1] + mat[3][1];
	out[2] = v[0] * mat[0][2] + v[1] * mat[1][2] + v[2] * mat[2][2] + mat[3][2];

	return out;
}

Vector matrix4x4::VectorITransform( const Vector &v ) const
{
	Vector iv, out;

	iv[0] = v[0] - mat[3][0];
	iv[1] = v[1] - mat[3][1];
	iv[2] = v[2] - mat[3][2];

	out[0] = iv[0] * mat[0][0] + iv[1] * mat[0][1] + iv[2] * mat[0][2];
	out[1] = iv[0] * mat[1][0] + iv[1] * mat[1][1] + iv[2] * mat[1][2];
	out[2] = iv[0] * mat[2][0] + iv[1] * mat[2][1] + iv[2] * mat[2][2];

	return out;
}

Vector matrix4x4::VectorRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[1][0] + v[2] * mat[2][0];
	out[1] = v[0] * mat[0][1] + v[1] * mat[1][1] + v[2] * mat[2][1];
	out[2] = v[0] * mat[0][2] + v[1] * mat[1][2] + v[2] * mat[2][2];

	return out;
}

Vector matrix4x4::VectorIRotate( const Vector &v ) const
{
	Vector out;

	out[0] = v[0] * mat[0][0] + v[1] * mat[0][1] + v[2] * mat[0][2];
	out[1] = v[0] * mat[1][0] + v[1] * mat[1][1] + v[2] * mat[1][2];
	out[2] = v[0] * mat[2][0] + v[1] * mat[2][1] + v[2] * mat[2][2];

	return out;
}

void matrix4x4::TransformPositivePlane( const mplane_t &in, mplane_t &out )
{
	float scale = sqrt( mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0] );
	float iscale = 1.0f / scale;
	mplane_t tmp = in;

	tmp.normal.x = (in.normal.x * mat[0][0] + in.normal.y * mat[1][0] + in.normal.z * mat[2][0]) * iscale;
	tmp.normal.y = (in.normal.x * mat[0][1] + in.normal.y * mat[1][1] + in.normal.z * mat[2][1]) * iscale;
	tmp.normal.z = (in.normal.x * mat[0][2] + in.normal.y * mat[1][2] + in.normal.z * mat[2][2]) * iscale;
	tmp.dist = in.dist * scale + ( tmp.normal.x * mat[3][0] + tmp.normal.y * mat[3][1] + tmp.normal.z * mat[3][2] );

	out = tmp;
}

void matrix4x4::TransformPositivePlane( const plane_t &in, plane_t &out )
{
	float scale = sqrt( mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0] );
	float iscale = 1.0f / scale;
	plane_t tmp = in;

	tmp.normal.x = (in.normal.x * mat[0][0] + in.normal.y * mat[1][0] + in.normal.z * mat[2][0]) * iscale;
	tmp.normal.y = (in.normal.x * mat[0][1] + in.normal.y * mat[1][1] + in.normal.z * mat[2][1]) * iscale;
	tmp.normal.z = (in.normal.x * mat[0][2] + in.normal.y * mat[1][2] + in.normal.z * mat[2][2]) * iscale;
	tmp.dist = in.dist * scale + ( tmp.normal.x * mat[3][0] + tmp.normal.y * mat[3][1] + tmp.normal.z * mat[3][2] );

	out = tmp;
}

void matrix4x4::TransformStandardPlane( const mplane_t &in, mplane_t &out )
{
	float scale = sqrt( mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0] );
	float iscale = 1.0f / scale;
	mplane_t tmp = in;

	tmp.normal.x = (in.normal.x * mat[0][0] + in.normal.y * mat[1][0] + in.normal.z * mat[2][0]) * iscale;
	tmp.normal.y = (in.normal.x * mat[0][1] + in.normal.y * mat[1][1] + in.normal.z * mat[2][1]) * iscale;
	tmp.normal.z = (in.normal.x * mat[0][2] + in.normal.y * mat[1][2] + in.normal.z * mat[2][2]) * iscale;
	tmp.dist = in.dist * scale - ( tmp.normal.x * mat[3][0] + tmp.normal.y * mat[3][1] + tmp.normal.z * mat[3][2] );

	out = tmp;
}

void matrix4x4::TransformStandardPlane( const plane_t &in, plane_t &out )
{
	float scale = sqrt( mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0] );
	float iscale = 1.0f / scale;
	plane_t tmp = in;

	tmp.normal.x = (in.normal.x * mat[0][0] + in.normal.y * mat[1][0] + in.normal.z * mat[2][0]) * iscale;
	tmp.normal.y = (in.normal.x * mat[0][1] + in.normal.y * mat[1][1] + in.normal.z * mat[2][1]) * iscale;
	tmp.normal.z = (in.normal.x * mat[0][2] + in.normal.y * mat[1][2] + in.normal.z * mat[2][2]) * iscale;
	tmp.dist = in.dist * scale - ( tmp.normal.x * mat[3][0] + tmp.normal.y * mat[3][1] + tmp.normal.z * mat[3][2] );

	out = tmp;
}

matrix4x4 matrix4x4 :: Invert( void ) const
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
	float scale = 1.0 / (mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);

	matrix4x4 out;

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out[0][0] = mat[0][0] * scale;
	out[0][1] = mat[1][0] * scale;
	out[0][2] = mat[2][0] * scale;
	out[1][0] = mat[0][1] * scale;
	out[1][1] = mat[1][1] * scale;
	out[1][2] = mat[2][1] * scale;
	out[2][0] = mat[0][2] * scale;
	out[2][1] = mat[1][2] * scale;
	out[2][2] = mat[2][2] * scale;

	// invert the translate
	out[3][0] = -(mat[3][0] * out[0][0] + mat[3][1] * out[1][0] + mat[3][2] * out[2][0]);
	out[3][1] = -(mat[3][0] * out[0][1] + mat[3][1] * out[1][1] + mat[3][2] * out[2][1]);
	out[3][2] = -(mat[3][0] * out[0][2] + mat[3][1] * out[1][2] + mat[3][2] * out[2][2]);

	// don't know if there's anything worth doing here
	out[0][3] = 0.0f;
	out[1][3] = 0.0f;
	out[2][3] = 0.0f;
	out[3][3] = 1.0f;

	return out;
}

matrix4x4 matrix4x4 :: InvertFull( void ) const
{
	float	*temp, *r[4];
	float	rtemp[4][8];
	float	s, m[4];
	matrix4x4 out;

	r[0] = rtemp[0];
	r[1] = rtemp[1];
	r[2] = rtemp[2];
	r[3] = rtemp[3];

	r[0][0] = mat[0][0];
	r[0][1] = mat[1][0];
	r[0][2] = mat[2][0];
	r[0][3] = mat[3][0];
	r[0][4] = 1.0;
	r[0][5] = 0.0;
	r[0][6] = 0.0;
	r[0][7] = 0.0;

	r[1][0] = mat[0][1];
	r[1][1] = mat[1][1];
	r[1][2] = mat[2][1];
	r[1][3] = mat[3][1];
	r[1][5] = 1.0;
	r[1][4] =	0.0;
	r[1][6] =	0.0;
	r[1][7] = 0.0;

	r[2][0] = mat[0][2];
	r[2][1] = mat[1][2];
	r[2][2] = mat[2][2];
	r[2][3] = mat[3][2];
	r[2][6] = 1.0;
	r[2][4] =	0.0;
	r[2][5] =	0.0;
	r[2][7] = 0.0;

	r[3][0] = mat[0][3];
	r[3][1] = mat[1][3];
	r[3][2] = mat[2][3];
	r[3][3] = mat[3][3];
	r[3][4] =	0.0;
	r[3][5] =	0.0;
	r[3][6] = 0.0;
	r[3][7] = 1.0;	

	if( fabs( r[3][0] ) > fabs( r[2][0] ))
	{
		temp = r[3];
		r[3] = r[2];
		r[2] = temp;
	}

	if( fabs( r[2][0] ) > fabs( r[1][0] ))
	{
		temp = r[2];
		r[2] = r[1];
		r[1] = temp;
	}

	if( fabs( r[1][0] ) > fabs( r[0][0] ))
	{
		temp = r[1];
		r[1] = r[0];
		r[0] = temp;
	}

	if( r[0][0] )
	{
		m[1] = r[1][0] / r[0][0];
		m[2] = r[2][0] / r[0][0];
		m[3] = r[3][0] / r[0][0];

		s = r[0][1];
		r[1][1] -= m[1] * s;
		r[2][1] -= m[2] * s;
		r[3][1] -= m[3] * s;

		s = r[0][2];
		r[1][2] -= m[1] * s;
		r[2][2] -= m[2] * s;
		r[3][2] -= m[3] * s;

		s = r[0][3];
		r[1][3] -= m[1] * s;
		r[2][3] -= m[2] * s;
		r[3][3] -= m[3] * s;

		s = r[0][4];
		if( s )
		{
			r[1][4] -= m[1] * s;
			r[2][4] -= m[2] * s;
			r[3][4] -= m[3] * s;
		}

		s = r[0][5];
		if( s )
		{
			r[1][5] -= m[1] * s;
			r[2][5] -= m[2] * s;
			r[3][5] -= m[3] * s;
		}

		s = r[0][6];
		if( s )
		{
			r[1][6] -= m[1] * s;
			r[2][6] -= m[2] * s;
			r[3][6] -= m[3] * s;
		}

		s = r[0][7];
		if( s )
		{
			r[1][7] -= m[1] * s;
			r[2][7] -= m[2] * s;
			r[3][7] -= m[3] * s;
		}

		if( fabs( r[3][1] ) > fabs( r[2][1] ))
		{
			temp = r[3];
			r[3] = r[2];
			r[2] = temp;
		}
		if( fabs( r[2][1] ) > fabs( r[1][1] ))
		{
			temp = r[2];
			r[2] = r[1];
			r[1] = temp;
		}

		if( r[1][1] )
		{
			m[2] = r[2][1] / r[1][1];
			m[3] = r[3][1] / r[1][1];
			r[2][2] -= m[2] * r[1][2];
			r[3][2] -= m[3] * r[1][2];
			r[2][3] -= m[2] * r[1][3];
			r[3][3] -= m[3] * r[1][3];

			s = r[1][4];
			if( s )
			{
				r[2][4] -= m[2] * s;
				r[3][4] -= m[3] * s;
			}

			s = r[1][5];
			if( s )
			{
				r[2][5] -= m[2] * s;
				r[3][5] -= m[3] * s;
			}

			s = r[1][6];
			if( s )
			{
				r[2][6] -= m[2] * s;
				r[3][6] -= m[3] * s;
			}

			s = r[1][7];
			if( s )
			{
				r[2][7] -= m[2] * s;
				r[3][7] -= m[3] * s;
			}

			if( fabs( r[3][2] ) > fabs( r[2][2] ))
			{
				temp = r[3];
				r[3] = r[2];
				r[2] = temp;
			}

			if( r[2][2] )
			{
				m[3] = r[3][2] / r[2][2];
				r[3][3] -= m[3] * r[2][3];
				r[3][4] -= m[3] * r[2][4];
				r[3][5] -= m[3] * r[2][5];
				r[3][6] -= m[3] * r[2][6];
				r[3][7] -= m[3] * r[2][7];

				if( r[3][3] )
				{
					s = 1.0 / r[3][3];
					r[3][4] *= s;
					r[3][5] *= s;
					r[3][6] *= s;
					r[3][7] *= s;

					m[2] = r[2][3];
					s = 1.0 / r[2][2];
					r[2][4] = s * (r[2][4] - r[3][4] * m[2]);
					r[2][5] = s * (r[2][5] - r[3][5] * m[2]);
					r[2][6] = s * (r[2][6] - r[3][6] * m[2]);
					r[2][7] = s * (r[2][7] - r[3][7] * m[2]);

					m[1] = r[1][3];
					r[1][4] -= r[3][4] * m[1];
					r[1][5] -= r[3][5] * m[1];
					r[1][6] -= r[3][6] * m[1];
					r[1][7] -= r[3][7] * m[1];

					m[0] = r[0][3];
					r[0][4] -= r[3][4] * m[0];
					r[0][5] -= r[3][5] * m[0];
					r[0][6] -= r[3][6] * m[0];
					r[0][7] -= r[3][7] * m[0];

					m[1] = r[1][2];
					s = 1.0 / r[1][1];
					r[1][4] = s * (r[1][4] - r[2][4] * m[1]);
					r[1][5] = s * (r[1][5] - r[2][5] * m[1]);
					r[1][6] = s * (r[1][6] - r[2][6] * m[1]);
					r[1][7] = s * (r[1][7] - r[2][7] * m[1]);

					m[0] = r[0][2];
					r[0][4] -= r[2][4] * m[0];
					r[0][5] -= r[2][5] * m[0];
					r[0][6] -= r[2][6] * m[0];
					r[0][7] -= r[2][7] * m[0];

					m[0] = r[0][1];
					s = 1.0 / r[0][0];
					r[0][4] = s * (r[0][4] - r[1][4] * m[0]);
					r[0][5] = s * (r[0][5] - r[1][5] * m[0]);
					r[0][6] = s * (r[0][6] - r[1][6] * m[0]);
					r[0][7] = s * (r[0][7] - r[1][7] * m[0]);

					out[0][0]	= r[0][4];
					out[0][1]	= r[1][4];
					out[0][2]	= r[2][4];
					out[0][3]	= r[3][4];
					out[1][0]	= r[0][5];
					out[1][1]	= r[1][5];
					out[1][2]	= r[2][5];
					out[1][3]	= r[3][5];
					out[2][0]	= r[0][6];
					out[2][1]	= r[1][6];
					out[2][2]	= r[2][6];
					out[2][3]	= r[3][6];
					out[3][0]	= r[0][7];
					out[3][1]	= r[1][7];
					out[3][2]	= r[2][7];
					out[3][3]	= r[3][7];

					return out;
				}
			}
		}
	}

	// failed
	return *this;
}

matrix4x4 matrix4x4 :: ConcatTransforms( const matrix4x4 mat2 )
{
	matrix4x4 out;

	out[0][0] = mat[0][0] * mat2[0][0] + mat[1][0] * mat2[0][1] + mat[2][0] * mat2[0][2];
	out[1][0] = mat[0][0] * mat2[1][0] + mat[1][0] * mat2[1][1] + mat[2][0] * mat2[1][2];
	out[2][0] = mat[0][0] * mat2[2][0] + mat[1][0] * mat2[2][1] + mat[2][0] * mat2[2][2];
	out[3][0] = mat[0][0] * mat2[3][0] + mat[1][0] * mat2[3][1] + mat[2][0] * mat2[3][2] + mat[3][0];
	out[0][1] = mat[0][1] * mat2[0][0] + mat[1][1] * mat2[0][1] + mat[2][1] * mat2[0][2];
	out[1][1] = mat[0][1] * mat2[1][0] + mat[1][1] * mat2[1][1] + mat[2][1] * mat2[1][2];
	out[2][1] = mat[0][1] * mat2[2][0] + mat[1][1] * mat2[2][1] + mat[2][1] * mat2[2][2];
	out[3][1] = mat[0][1] * mat2[3][0] + mat[1][1] * mat2[3][1] + mat[2][1] * mat2[3][2] + mat[3][1];
	out[0][2] = mat[0][2] * mat2[0][0] + mat[1][2] * mat2[0][1] + mat[2][2] * mat2[0][2];
	out[1][2] = mat[0][2] * mat2[1][0] + mat[1][2] * mat2[1][1] + mat[2][2] * mat2[1][2];
	out[2][2] = mat[0][2] * mat2[2][0] + mat[1][2] * mat2[2][1] + mat[2][2] * mat2[2][2];
	out[3][2] = mat[0][2] * mat2[3][0] + mat[1][2] * mat2[3][1] + mat[2][2] * mat2[3][2] + mat[3][2];

	// not used for concat transforms
	out[0][3] = 0.0f;
	out[1][3] = 0.0f;
	out[2][3] = 0.0f;
	out[3][3] = 1.0f;

	return out;
}

matrix4x4 matrix4x4 :: Concat( const matrix4x4 mat2 )
{
	matrix4x4 out;

	out[0][0] = mat[0][0] * mat2[0][0] + mat[1][0] * mat2[0][1] + mat[2][0] * mat2[0][2] + mat[3][0] * mat2[0][3];
	out[1][0] = mat[0][0] * mat2[1][0] + mat[1][0] * mat2[1][1] + mat[2][0] * mat2[1][2] + mat[3][0] * mat2[1][3];
	out[2][0] = mat[0][0] * mat2[2][0] + mat[1][0] * mat2[2][1] + mat[2][0] * mat2[2][2] + mat[3][0] * mat2[2][3];
	out[3][0] = mat[0][0] * mat2[3][0] + mat[1][0] * mat2[3][1] + mat[2][0] * mat2[3][2] + mat[3][0] * mat2[3][3];
	out[0][1] = mat[0][1] * mat2[0][0] + mat[1][1] * mat2[0][1] + mat[2][1] * mat2[0][2] + mat[3][1] * mat2[0][3];
	out[1][1] = mat[0][1] * mat2[1][0] + mat[1][1] * mat2[1][1] + mat[2][1] * mat2[1][2] + mat[3][1] * mat2[1][3];
	out[2][1] = mat[0][1] * mat2[2][0] + mat[1][1] * mat2[2][1] + mat[2][1] * mat2[2][2] + mat[3][1] * mat2[2][3];
	out[3][1] = mat[0][1] * mat2[3][0] + mat[1][1] * mat2[3][1] + mat[2][1] * mat2[3][2] + mat[3][1] * mat2[3][3];
	out[0][2] = mat[0][2] * mat2[0][0] + mat[1][2] * mat2[0][1] + mat[2][2] * mat2[0][2] + mat[3][2] * mat2[0][3];
	out[1][2] = mat[0][2] * mat2[1][0] + mat[1][2] * mat2[1][1] + mat[2][2] * mat2[1][2] + mat[3][2] * mat2[1][3];
	out[2][2] = mat[0][2] * mat2[2][0] + mat[1][2] * mat2[2][1] + mat[2][2] * mat2[2][2] + mat[3][2] * mat2[2][3];
	out[3][2] = mat[0][2] * mat2[3][0] + mat[1][2] * mat2[3][1] + mat[2][2] * mat2[3][2] + mat[3][2] * mat2[3][3];
	out[0][3] = mat[0][3] * mat2[0][0] + mat[1][3] * mat2[0][1] + mat[2][3] * mat2[0][2] + mat[3][3] * mat2[0][3];
	out[1][3] = mat[0][3] * mat2[1][0] + mat[1][3] * mat2[1][1] + mat[2][3] * mat2[1][2] + mat[3][3] * mat2[1][3];
	out[2][3] = mat[0][3] * mat2[2][0] + mat[1][3] * mat2[2][1] + mat[2][3] * mat2[2][2] + mat[3][3] * mat2[2][3];
	out[3][3] = mat[0][3] * mat2[3][0] + mat[1][3] * mat2[3][1] + mat[2][3] * mat2[3][2] + mat[3][3] * mat2[3][3];

	return out;
}

// from class matrix3x3 to class matrix4x4
matrix4x4& matrix4x4 :: operator=(const matrix3x3 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][0] = vOther[1][0];
	mat[2][0] = vOther[2][0];
	mat[3][0] = 0.0f;
	mat[0][1] = vOther[0][1];
	mat[1][1] = vOther[1][1];
	mat[2][1] = vOther[2][1];
	mat[3][1] = 0.0f;
	mat[0][2] = vOther[0][2];
	mat[1][2] = vOther[1][2];
	mat[2][2] = vOther[2][2];
	mat[3][2] = 0.0f;
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;

	return *this;
}

// from class matrix3x4 to class matrix4x4
matrix4x4& matrix4x4 :: operator=(const matrix3x4 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][0] = vOther[1][0];
	mat[2][0] = vOther[2][0];
	mat[3][0] = vOther[3][0];
	mat[0][1] = vOther[0][1];
	mat[1][1] = vOther[1][1];
	mat[2][1] = vOther[2][1];
	mat[3][1] = vOther[3][1];
	mat[0][2] = vOther[0][2];
	mat[1][2] = vOther[1][2];
	mat[2][2] = vOther[2][2];
	mat[3][2] = vOther[3][2];
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;

	return *this;
}

// from class matrix3x4 to class matrix4x4
matrix4x4& matrix4x4 :: operator=(const matrix4x4 &vOther)
{
	mat[0][0] = vOther[0][0];
	mat[1][0] = vOther[1][0];
	mat[2][0] = vOther[2][0];
	mat[3][0] = vOther[3][0];
	mat[0][1] = vOther[0][1];
	mat[1][1] = vOther[1][1];
	mat[2][1] = vOther[2][1];
	mat[3][1] = vOther[3][1];
	mat[0][2] = vOther[0][2];
	mat[1][2] = vOther[1][2];
	mat[2][2] = vOther[2][2];
	mat[3][2] = vOther[3][2];
	mat[0][3] = vOther[0][3];
	mat[1][3] = vOther[1][3];
	mat[2][3] = vOther[2][3];
	mat[3][3] = vOther[3][3];

	return *this;
}

void matrix4x4::CreateProjection( float xMax, float xMin, float yMax, float yMin, float zNear, float zFar )
{
	mat[0][0] = ( 2.0f * zNear ) / ( xMax - xMin );
	mat[1][1] = ( 2.0f * zNear ) / ( yMax - yMin );
	mat[2][2] = -( zFar + zNear ) / ( zFar - zNear );
	mat[3][3] = mat[0][1] = mat[1][0] = mat[3][0] = mat[0][3] = mat[3][1] = mat[1][3] = 0.0f;

	mat[0][2] = 0.0f;
	mat[1][2] = 0.0f;
	mat[2][0] = ( xMax + xMin ) / ( xMax - xMin );
	mat[2][1] = ( yMax + yMin ) / ( yMax - yMin );
	mat[2][3] = -1.0f;
	mat[3][2] = -( 2.0f * zFar * zNear ) / ( zFar - zNear );
}

void matrix4x4::CreateOrtho( float xLeft, float xRight, float yBottom, float yTop, float zNear, float zFar )
{
	mat[0][0] = 2.0f / (xRight - xLeft);
	mat[1][1] = 2.0f / (yTop - yBottom);
	mat[2][2] = -2.0f / (zFar - zNear);
	mat[3][3] = 1.0f;
	mat[0][1] = mat[1][0] = mat[2][0] = mat[2][1] = mat[0][3] = mat[1][3] = mat[2][3] = 0.0f;

	mat[0][2] = 0.0f;
	mat[1][2] = 0.0f;
	mat[3][0] = -(xRight + xLeft) / (xRight - xLeft);
	mat[3][1] = -(yTop + yBottom) / (yTop - yBottom);
	mat[3][2] = -(zFar + zNear) / (zFar - zNear);
}

void matrix4x4::CreateModelview( void )
{
	mat[0][0] = mat[1][1] = mat[2][2] = 0.0f;
	mat[3][0] = mat[0][3] = 0.0f;
	mat[3][1] = mat[1][3] = 0.0f;
	mat[3][2] = mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
	mat[0][1] = mat[2][0] = mat[1][2] = 0.0f;
	mat[0][2] = mat[1][0] = -1.0f;
	mat[2][1] = 1.0f;
}

void matrix4x4::CreateTranslate( float x, float y, float z )
{
	mat[0][0] = 1.0f;
	mat[1][0] = 0.0f;
	mat[2][0] = 0.0f;
	mat[3][0] = x;
	mat[0][1] = 0.0f;
	mat[1][1] = 1.0f;
	mat[2][1] = 0.0f;
	mat[3][1] = y;
	mat[0][2] = 0.0f;
	mat[1][2] = 0.0f;
	mat[2][2] = 1.0f;
	mat[3][2] = z;
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
}

void matrix4x4::CreateRotate( float angle, float x, float y, float z )
{
	float	len, c, s;

	len = x * x + y * y + z * z;
	if( len != 0.0f ) len = 1.0f / sqrt( len );

	x *= len;
	y *= len;
	z *= len;

	angle *= (float)(-M_PI / 180.0f);
	SinCos( angle, &s, &c );

	mat[0][0] = x * x + c * (1 - x * x);
	mat[1][0] = x * y * (1 - c) + z * s;
	mat[2][0] = z * x * (1 - c) - y * s;
	mat[3][0] = 0.0f;
	mat[0][1] = x * y * (1 - c) - z * s;
	mat[1][1] = y * y + c * (1 - y * y);
	mat[2][1] = y * z * (1 - c) + x * s;
	mat[3][1] = 0.0f;
	mat[0][2] = z * x * (1 - c) + y * s;
	mat[1][2] = y * z * (1 - c) - x * s;
	mat[2][2] = z * z + c * (1 - z * z);
	mat[3][2] = 0.0f;
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
}

void matrix4x4::CreateScale( float scale )
{
	mat[0][0] = scale;
	mat[1][0] = 0.0f;
	mat[2][0] = 0.0f;
	mat[3][0] = 0.0f;
	mat[0][1] = 0.0f;
	mat[1][1] = scale;
	mat[2][1] = 0.0f;
	mat[3][1] = 0.0f;
	mat[0][2] = 0.0f;
	mat[1][2] = 0.0f;
	mat[2][2] = scale;
	mat[3][2] = 0.0f;
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
}

void matrix4x4::CreateScale( float x, float y, float z )
{
	mat[0][0] = x;
	mat[1][0] = 0.0f;
	mat[2][0] = 0.0f;
	mat[3][0] = 0.0f;
	mat[0][1] = 0.0f;
	mat[1][1] = y;
	mat[2][1] = 0.0f;
	mat[3][1] = 0.0f;
	mat[0][2] = 0.0f;
	mat[1][2] = 0.0f;
	mat[2][2] = z;
	mat[3][2] = 0.0f;
	mat[0][3] = 0.0f;
	mat[1][3] = 0.0f;
	mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
}

matrix4x4 matrix4x4::QuakeToNewton( void ) const
{
	matrix4x4 out;

	out[0][0] = mat[0][0];
	out[0][2] = mat[0][1];
	out[0][1] = mat[0][2];
	out[0][3] = mat[0][3];
	out[1][0] = mat[1][0];
	out[1][2] = mat[1][1];
	out[1][1] = mat[1][2];
	out[1][3] = mat[1][3];
	out[2][0] = mat[2][0];
	out[2][2] = mat[2][1];
	out[2][1] = -mat[2][2];
	out[2][3] = mat[2][3];
	out[3][0] = INCH2METER( mat[3][0] );
	out[3][2] = INCH2METER( mat[3][1] );
	out[3][1] = INCH2METER( mat[3][2] );
	out[3][3] = mat[3][3];

	return out;
}

matrix4x4 matrix4x4::NewtonToQuake( void ) const
{
	matrix4x4 out;

	out[0][0] = mat[0][0];
	out[0][1] = mat[0][2];
	out[0][2] = mat[0][1];
	out[0][3] = mat[0][3];
	out[1][0] = mat[1][0];
	out[1][1] = mat[1][2];
	out[1][2] = mat[1][1];
	out[1][3] = mat[1][3];
	out[2][0] = mat[2][0];
	out[2][1] = mat[2][2];
	out[2][2] = -mat[2][1];
	out[2][3] = mat[2][3];
	out[3][0] = METER2INCH( mat[3][0] );
	out[3][1] = METER2INCH( mat[3][2] );
	out[3][2] = METER2INCH( mat[3][1] );
	out[3][3] = mat[3][3];

	return out;
}