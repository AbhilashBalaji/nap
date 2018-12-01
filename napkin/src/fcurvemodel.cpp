#include <napqt/qtutils.h>
#include "fcurvemodel.h"

using namespace napkin;


const QString FCurve::name() const
{
	return QString::fromStdString(mCurve.mID);
}

int FCurve::pointCount() const
{
	return static_cast<int>(mCurve.mPoints.size());
}

void FCurve::removePoints(const QList<int>& indices)
{
	QList<int> indicesSorted = indices;
	qSort(indicesSorted);
	for (int i = indicesSorted.size() - 1; i >= 0; i--)
	{
		mCurve.mPoints.erase(mCurve.mPoints.begin() + indicesSorted[i]);
	}
	mCurve.invalidate();
	pointsRemoved(indicesSorted);
}

void FCurve::addPoint(qreal time, qreal value)
{
	int index = static_cast<int>(mCurve.mPoints.size());
	mCurve.mPoints.emplace_back(nap::math::FCurvePoint<float, float>({time, value}, {-0.1, 0}, {0.1, 0}));
	mCurve.invalidate();
	pointsAdded({index});
}

qreal FCurve::evaluate(qreal t) const
{
	return mCurve.evaluate(static_cast<const float&>(t));
}

const QPointF FCurve::pos(int pointIndex) const
{
	const auto& pos = mCurve.mPoints[pointIndex].mPos;
	return {pos.mTime, pos.mValue};
}

const QPointF FCurve::inTangent(int pointIndex) const
{
	const auto& tan = mCurve.mPoints[pointIndex].mInTan;
	return {tan.mTime, tan.mValue};
}

const QPointF FCurve::outTangent(int pointIndex) const
{
	const auto& tan = mCurve.mPoints[pointIndex].mOutTan;
	return {tan.mTime, tan.mValue};
}

const napqt::AbstractCurve::InterpType FCurve::interpolation(int pointIndex) const
{
	const auto& interp = mCurve.mPoints[pointIndex].mInterp;
	return mInterpMap[interp];
}

void FCurve::setInterpolation(int pointIndex, const napqt::AbstractCurve::InterpType& interp)
{
	nap::math::FCurveInterp destInterp = napqt::keyFromValue(mInterpMap, interp, nap::math::FCurveInterp::Bezier);
	mCurve.mPoints[pointIndex].mInterp = destInterp;
	pointsChanged({pointIndex}, true);
}

const bool FCurve::tangentsAligned(int pointIndex) const
{
	return mCurve.mPoints[pointIndex].mTangentsAligned;
}

void FCurve::setTangentsAligned(int pointIndex, bool b)
{
	mCurve.mPoints[pointIndex].mTangentsAligned = b;
	pointsChanged({pointIndex}, true);
}


void FCurve::movePoints(const QMap<int, QPointF>& positions, bool finished)
{
	for (auto it = positions.begin(); it != positions.end(); it++)
	{
		setComplexValue(mCurve.mPoints[it.key()].mPos, it.value());
	}
	pointsChanged(positions.keys(), finished);

	// Points need to be sorted for proper evaluation
	mCurve.invalidate();
}

void FCurve::setComplexValue(nap::math::FComplex<float, float>& c, const QPointF& p)
{
	c.mTime = static_cast<float>(p.x());
	c.mValue = static_cast<float>(p.y());
}

void FCurve::moveTangents(const QMap<int, QPointF>& inTangents, const QMap<int, QPointF>& outTangents, bool finished)
{
	QList<int> changed;
	for (auto it = inTangents.begin(); it != inTangents.end(); it++)
	{
		int index = it.key();
		setComplexValue(mCurve.mPoints[index].mInTan, it.value());

		if (!changed.contains(index))
			changed << index;
	}
	for (auto it = outTangents.begin(); it != outTangents.end(); it++)
	{
		int index = it.key();
		setComplexValue(mCurve.mPoints[index].mOutTan, it.value());
		if (!changed.contains(index))
			changed << index;
	}

	pointsChanged(changed, finished);
}

const PropertyPath FCurve::pointPath(int pointIndex)
{
	std::string pointPath = QString("Points/%1").arg(pointIndex).toStdString();
	return {sourceCurve(), pointPath};
}


FloatFCurveModel::FloatFCurveModel(nap::math::FunctionCurve<float, float>& curve) : napqt::AbstractCurveModel(), mCurve(curve) {}

int FloatFCurveModel::curveCount() const
{
	return 1;
}

napqt::AbstractCurve* FloatFCurveModel::curve(int index) const
{
	return const_cast<FCurve*>(&mCurve);
}
