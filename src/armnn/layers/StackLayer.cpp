//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//
#include "StackLayer.hpp"
#include "LayerCloneBase.hpp"

#include <armnn/TypesUtils.hpp>
#include <backendsCommon/WorkloadData.hpp>
#include <backendsCommon/WorkloadFactory.hpp>

#include <queue>

namespace armnn
{

StackLayer::StackLayer(const StackDescriptor& param, const char* name)
    : LayerWithParameters(param.m_NumInputs, 1, LayerType::Stack, param, name)
{
}

std::unique_ptr<IWorkload> StackLayer::CreateWorkload(const Graph& graph, const IWorkloadFactory& factory) const
{
    StackQueueDescriptor descriptor;
    return factory.CreateStack(descriptor, PrepInfoAndDesc(descriptor, graph));
}

StackLayer* StackLayer::Clone(Graph& graph) const
{
    return CloneBase<StackLayer>(graph, m_Param, GetName());
}

std::vector<TensorShape> StackLayer::InferOutputShapes(const std::vector<TensorShape>& inputShapes) const
{
    const TensorShape& inputShape = m_Param.m_InputShape;
    const unsigned int inputNumDimensions = inputShape.GetNumDimensions();
    const unsigned int axis = m_Param.m_Axis;

    BOOST_ASSERT(axis <= inputNumDimensions);

    unsigned int dimensionSizes[inputNumDimensions + 1];
    for (unsigned int i = 0; i < axis; ++i)
    {
        dimensionSizes[i] = inputShape[i];
    }

    dimensionSizes[axis] = m_Param.m_NumInputs;

    for (unsigned int i = axis + 1; i < inputNumDimensions + 1; ++i)
    {
        dimensionSizes[i] = inputShape[i-1];
    }

    TensorShape targetShape = TensorShape(inputNumDimensions + 1, dimensionSizes);

    return std::vector<TensorShape>({ targetShape });
}

void StackLayer::ValidateTensorShapesFromInputs()
{
    // Validates Stack layer.
    ConditionalThrowIfNotEqual<LayerValidationException>(
        "StackLayer: Num Input Slots must match Num Inputs.",
        m_Param.m_NumInputs,
        GetNumInputSlots());

    VerifyLayerConnections(m_Param.m_NumInputs, CHECK_LOCATION());

    // Constructs and validates input shapes
    std::vector<TensorShape> inputShapes;
    for (unsigned int i = 0; i < GetNumInputSlots(); ++i)
    {
        TensorShape inputShape = GetInputSlot(i).GetConnection()->GetTensorInfo().GetShape();
        if (inputShape != m_Param.m_InputShape)
        {
            throw LayerValidationException("StackLayer: TensorShape set on InputSlot[" +
                                           std::to_string(i) +
                                           "] does not match defined input shape");
        }
        inputShapes.push_back(inputShape);
    }

    auto inferredShapes = InferOutputShapes(inputShapes);

    BOOST_ASSERT(inferredShapes.size() == 1);

    ConditionalThrowIfNotEqual<LayerValidationException>(
        "StackLayer: TensorShape set on OutputSlot[0] does not match the inferred shape.",
        GetOutputSlot(0).GetTensorInfo().GetShape(),
        inferredShapes[0]);
}

void StackLayer::Accept(ILayerVisitor& visitor) const
{
    visitor.VisitStackLayer(this, GetParameters(), GetName());
}

} // namespace armnn armnn