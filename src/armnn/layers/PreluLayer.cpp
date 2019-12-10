//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "PreluLayer.hpp"

#include "LayerCloneBase.hpp"

#include <backendsCommon/WorkloadData.hpp>
#include <backendsCommon/WorkloadFactory.hpp>
#include <backendsCommon/CpuTensorHandle.hpp>

namespace armnn
{

PreluLayer::PreluLayer(const char* name)
    : Layer(2, 1, LayerType::Prelu, name)
{}

std::unique_ptr<IWorkload> PreluLayer::CreateWorkload(const IWorkloadFactory& factory) const
{
    PreluQueueDescriptor descriptor;

    return factory.CreatePrelu(descriptor, PrepInfoAndDesc(descriptor));
}

PreluLayer* PreluLayer::Clone(Graph& graph) const
{
    auto layer = CloneBase<PreluLayer>(graph, GetName());

    return std::move(layer);
}

std::vector<TensorShape> PreluLayer::InferOutputShapes(const std::vector<TensorShape>& inputShapes) const
{
    BOOST_ASSERT(inputShapes.size() == 2);

    const TensorShape& inputShape = inputShapes[0];
    const TensorShape& alphaShape = inputShapes[1];

    const unsigned int inputShapeDimensions = inputShape.GetNumDimensions();
    const unsigned int alphaShapeDimensions = alphaShape.GetNumDimensions();

    BOOST_ASSERT(inputShapeDimensions > 0);
    BOOST_ASSERT(alphaShapeDimensions > 0);

    // The size of the output is the maximum size along each dimension of the input operands,
    // it starts with the trailing dimensions, and works its way forward

    unsigned int outputDimensions = std::max(inputShapeDimensions, alphaShapeDimensions);

    TensorShape outputShape(outputDimensions);

    int inputShapeIndex = boost::numeric_cast<int>(inputShapeDimensions) - 1;
    int alphaShapeIndex = boost::numeric_cast<int>(alphaShapeDimensions) - 1;
    unsigned int outputShapeIndex = outputDimensions - 1;

    // Loop backwards through the common part of the shapes
    while (inputShapeIndex >= 0 && alphaShapeIndex >= 0)
    {
        unsigned int inputDimension = inputShape[boost::numeric_cast<unsigned int>(inputShapeIndex)];
        unsigned int alphaDimension = alphaShape[boost::numeric_cast<unsigned int>(alphaShapeIndex)];

        // Check that the inputs are broadcast compatible
        BOOST_ASSERT_MSG(inputDimension == alphaDimension || inputDimension == 1 || alphaDimension == 1,
                         "PreluLayer: Dimensions should either match or one should be of size 1");

        outputShape[outputShapeIndex] = std::max(inputDimension, alphaDimension);

        inputShapeIndex--;
        alphaShapeIndex--;
        outputShapeIndex--;
    }

    // Loop backwards through the remaing part of the input shape (if any)
    while (inputShapeIndex >= 0)
    {
        outputShape[outputShapeIndex] = inputShape[boost::numeric_cast<unsigned int>(inputShapeIndex)];

        inputShapeIndex--;
        outputShapeIndex--;
    }

    // Loop backwards through the remaing part of the alpha shape (if any)
    while (alphaShapeIndex >= 0)
    {
        outputShape[outputShapeIndex] = alphaShape[boost::numeric_cast<unsigned int>(alphaShapeIndex)];

        alphaShapeIndex--;
        outputShapeIndex--;
    }

    return { outputShape };
}

void PreluLayer::ValidateTensorShapesFromInputs()
{
    VerifyLayerConnections(2, CHECK_LOCATION());

    std::vector<TensorShape> inferredShapes = InferOutputShapes(
    {
        GetInputSlot(0).GetConnection()->GetTensorInfo().GetShape(),
        GetInputSlot(1).GetConnection()->GetTensorInfo().GetShape()
    });

    BOOST_ASSERT(inferredShapes.size() == 1);

    ConditionalThrowIfNotEqual<LayerValidationException>(
        "PreluLayer: TensorShape set on OutputSlot[0] does not match the inferred shape.",
        GetOutputSlot(0).GetTensorInfo().GetShape(),
        inferredShapes[0]);
}

void PreluLayer::Accept(ILayerVisitor& visitor) const
{
    visitor.VisitPreluLayer(this, GetName());
}

} // namespace armnn
