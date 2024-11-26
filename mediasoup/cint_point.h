
/***********************************************************************************************
created: 		2022-01-20

author:			chensong

purpose:		cint_point
输赢不重要，答案对你们有什么意义才重要。

光阴者，百代之过客也，唯有奋力奔跑，方能生风起时，是时代造英雄，英雄存在于时代。或许世人道你轻狂，可你本就年少啊。 看护好，自己的理想和激情。


我可能会遇到很多的人，听他们讲好2多的故事，我来写成故事或编成歌，用我学来的各种乐器演奏它。
然后还可能在一个国家遇到一个心仪我的姑娘，她可能会被我帅气的外表捕获，又会被我深邃的内涵吸引，在某个下雨的夜晚，她会全身淋透然后要在我狭小的住处换身上的湿衣服。
3小时候后她告诉我她其实是这个国家的公主，她愿意向父皇求婚。我不得已告诉她我是穿越而来的男主角，我始终要回到自己的世界。
然后我的身影慢慢消失，我看到她眼里的泪水，心里却没有任何痛苦，我才知道，原来我的心被丢掉了，我游历全世界的原因，就是要找回自己的本心。
于是我开始有意寻找各种各样失去心的人，我变成一块砖头，一颗树，一滴水，一朵白云，去听大家为什么会失去自己的本心。
我发现，刚出生的宝宝，本心还在，慢慢的，他们的本心就会消失，收到了各种黑暗之光的侵蚀。
从一次争论，到嫉妒和悲愤，还有委屈和痛苦，我看到一只只无形的手，把他们的本心扯碎，蒙蔽，偷走，再也回不到主人都身边。
我叫他本心猎手。他可能是和宇宙同在的级别 但是我并不害怕，我仔细回忆自己平淡的一生 寻找本心猎手的痕迹。
沿着自己的回忆，一个个的场景忽闪而过，最后发现，我的本心，在我写代码的时候，会回来。
安静，淡然，代码就是我的一切，写代码就是我本心回归的最好方式，我还没找到本心猎手，但我相信，顺着这个线索，我一定能顺藤摸瓜，把他揪出来。
************************************************************************************************/

#ifndef _C_INT_POINT_H_
#define _C_INT_POINT_H_
#include "cnet_types.h"


namespace chen {

	/**
	* Structure for integer points in 2-d space.
	*
	* @todo Docs: The operators need better documentation, i.e. what does it mean to divide a point?
	*/
	struct FIntPoint
	{
		/** Holds the point's x-coordinate. */
		int64 X;

		/** Holds the point's y-coordinate. */
		int64 Y;

	//public:

	//	/** An integer point with zeroed values. */
	//	 static const FIntPoint ZeroValue;

	//	/** An integer point with INDEX_NONE values. */
	//	 static const FIntPoint NoneValue;

	//public:

	//	/** Default constructor (no initialization). */
		FIntPoint() :X(0), Y(0) {}

	//	/**
	//	* Create and initialize a new instance with the specified coordinates.
	//	*
	//	* @param InX The x-coordinate.
	//	* @param InY The y-coordinate.
	//	*/
	//	FIntPoint(int32 InX, int32 InY);

	//	/**
	//	* Create and initialize a new instance with a single int.
	//	* Both X and Y will be initialized to this value
	//	*
	//	* @param InXY The x and y-coordinate.
	//	*/
	//	FIntPoint(int32 InXY);

	//	/**
	//	* Create and initialize a new instance to zero.
	//	*
	//	* @param EForceInit Force init enum
	//	*/
	//	explicit  FIntPoint(EForceInit);

	//public:

	//	/**
	//	* Get specific component of a point.
	//	*
	//	* @param PointIndex Index of point component.
	//	* @return const reference to component.
	//	*/
	//	const int32& operator()(int32 PointIndex) const;

	//	/**
	//	* Get specific component of a point.
	//	*
	//	* @param PointIndex Index of point component
	//	* @return reference to component.
	//	*/
	//	int32& operator()(int32 PointIndex);

	//	/**
	//	* Compare two points for equality.
	//	*
	//	* @param Other The other int point being compared.
	//	* @return true if the points are equal, false otherwise.
	//	*/
	//	bool operator==(const FIntPoint& Other) const;

	//	/**
	//	* Compare two points for inequality.
	//	*
	//	* @param Other The other int point being compared.
	//	* @return true if the points are not equal, false otherwise.
	//	*/
	//	bool operator!=(const FIntPoint& Other) const;

	//	/**
	//	* Scale this point.
	//	*
	//	* @param Scale What to multiply the point by.
	//	* @return Reference to this point after multiplication.
	//	*/
	//	FIntPoint& operator*=(int32 Scale);

	//	/**
	//	* Divide this point by a scalar.
	//	*
	//	* @param Divisor What to divide the point by.
	//	* @return Reference to this point after division.
	//	*/
	//	FIntPoint& operator/=(int32 Divisor);

	//	/**
	//	* Add another point component-wise to this point.
	//	*
	//	* @param Other The point to add to this point.
	//	* @return Reference to this point after addition.
	//	*/
	//	FIntPoint& operator+=(const FIntPoint& Other);

	//	/**
	//	* Multiply another point component-wise from this point.
	//	*
	//	* @param Other The point to multiply with this point.
	//	* @return Reference to this point after multiplication.
	//	*/
	//	FIntPoint& operator*=(const FIntPoint& Other);

	//	/**
	//	* Subtract another point component-wise from this point.
	//	*
	//	* @param Other The point to subtract from this point.
	//	* @return Reference to this point after subtraction.
	//	*/
	//	FIntPoint& operator-=(const FIntPoint& Other);

	//	/**
	//	* Divide this point component-wise by another point.
	//	*
	//	* @param Other The point to divide with.
	//	* @return Reference to this point after division.
	//	*/
	//	FIntPoint& operator/=(const FIntPoint& Other);

	//	/**
	//	* Assign another point to this one.
	//	*
	//	* @param Other The point to assign this point from.
	//	* @return Reference to this point after assignment.
	//	*/
	//	FIntPoint& operator=(const FIntPoint& Other);

	//	/**
	//	* Get the result of scaling on this point.
	//	*
	//	* @param Scale What to multiply the point by.
	//	* @return A new scaled int point.
	//	*/
	//	FIntPoint operator*(int32 Scale) const;

	//	/**
	//	* Get the result of division on this point.
	//	*
	//	* @param Divisor What to divide the point by.
	//	* @return A new divided int point.
	//	*/
	//	FIntPoint operator/(int32 Divisor) const;

	//	/**
	//	* Get the result of addition on this point.
	//	*
	//	* @param Other The other point to add to this.
	//	* @return A new combined int point.
	//	*/
	//	FIntPoint operator+(const FIntPoint& Other) const;

	//	/**
	//	* Get the result of subtraction from this point.
	//	*
	//	* @param Other The other point to subtract from this.
	//	* @return A new subtracted int point.
	//	*/
	//	FIntPoint operator-(const FIntPoint& Other) const;

	//	/**
	//	* Get the result of multiplication on this point.
	//	*
	//	* @param Other The point to multiply with this point.
	//	* @return A new multiplied int point
	//	*/
	//	FIntPoint operator*(const FIntPoint& Other) const;

	//	/**
	//	* Get the result of division on this point.
	//	*
	//	* @param Other The other point to subtract from this.
	//	* @return A new subtracted int point.
	//	*/
	//	FIntPoint operator/(const FIntPoint& Other) const;

	//	/**
	//	* Get specific component of the point.
	//	*
	//	* @param Index the index of point component
	//	* @return reference to component.
	//	*/
	//	int32& operator[](int32 Index);

	//	/**
	//	* Get specific component of the point.
	//	*
	//	* @param Index the index of point component
	//	* @return copy of component value.
	//	*/
	//	int32 operator[](int32 Index) const;

	//public:

	//	/**
	//	* Get the component-wise min of two points.
	//	*
	//	* @see ComponentMax, GetMax
	//	*/
	//	FORCEINLINE FIntPoint ComponentMin(const FIntPoint& Other) const;

	//	/**
	//	* Get the component-wise max of two points.
	//	*
	//	* @see ComponentMin, GetMin
	//	*/
	//	FORCEINLINE FIntPoint ComponentMax(const FIntPoint& Other) const;

	//	/**
	//	* Get the larger of the point's two components.
	//	*
	//	* @return The maximum component of the point.
	//	* @see GetMin, Size, SizeSquared
	//	*/
	//	int32 GetMax() const;

	//	/**
	//	* Get the smaller of the point's two components.
	//	*
	//	* @return The minimum component of the point.
	//	* @see GetMax, Size, SizeSquared
	//	*/
	//	int32 GetMin() const;

	//	/**
	//	* Get the distance of this point from (0,0).
	//	*
	//	* @return The distance of this point from (0,0).
	//	* @see GetMax, GetMin, SizeSquared
	//	*/
	//	int32 Size() const;

	//	/**
	//	* Get the squared distance of this point from (0,0).
	//	*
	//	* @return The squared distance of this point from (0,0).
	//	* @see GetMax, GetMin, Size
	//	*/
	//	int32 SizeSquared() const;

	//	/**
	//	* Get a textual representation of this point.
	//	*
	//	* @return A string describing the point.
	//	*/
	//	FString ToString() const;

	//public:

	//	/**
	//	* Divide an int point and round up the result.
	//	*
	//	* @param lhs The int point being divided.
	//	* @param Divisor What to divide the int point by.
	//	* @return A new divided int point.
	//	* @see DivideAndRoundDown
	//	*/
	//	static FIntPoint DivideAndRoundUp(FIntPoint lhs, int32 Divisor);
	//	static FIntPoint DivideAndRoundUp(FIntPoint lhs, FIntPoint Divisor);

	//	/**
	//	* Divide an int point and round down the result.
	//	*
	//	* @param lhs The int point being divided.
	//	* @param Divisor What to divide the int point by.
	//	* @return A new divided int point.
	//	* @see DivideAndRoundUp
	//	*/
	//	static FIntPoint DivideAndRoundDown(FIntPoint lhs, int32 Divisor);

	//	/**
	//	* Get number of components point has.
	//	*
	//	* @return number of components point has.
	//	*/
	//	static int32 Num();

	//public:

	//	/**
	//	* Serialize the point.
	//	*
	//	* @param Ar The archive to serialize into.
	//	* @param Point The point to serialize.
	//	* @return Reference to the Archive after serialization.
	//	*/
	//	friend FArchive& operator<<(FArchive& Ar, FIntPoint& Point)
	//	{
	//		return Ar << Point.X << Point.Y;
	//	}

	//	/**
	//	* Serialize the point.
	//	*
	//	* @param Slot The structured archive slot to serialize into.
	//	* @param Point The point to serialize.
	//	*/
	//	friend void operator<<(FStructuredArchive::FSlot Slot, FIntPoint& Point)
	//	{
	//		FStructuredArchive::FRecord Record = Slot.EnterRecord();
	//		Record << SA_VALUE(TEXT("X"), Point.X) << SA_VALUE(TEXT("Y"), Point.Y);
	//	}

	//	/**
	//	* Serialize the point.
	//	*
	//	* @param Ar The archive to serialize into.
	//	* @return true on success, false otherwise.
	//	*/
	//	bool Serialize(FArchive& Ar)
	//	{
	//		Ar << *this;
	//		return true;
	//	}

	//	/**
	//	* Serialize the point.
	//	*
	//	* @param Slot The structured archive slot to serialize into.
	//	* @return true on success, false otherwise.
	//	*/
	//	bool Serialize(FStructuredArchive::FSlot Slot)
	//	{
	//		Slot << *this;
	//		return true;
	//	}
	};

}
#endif // _C_IN_POINT_H_