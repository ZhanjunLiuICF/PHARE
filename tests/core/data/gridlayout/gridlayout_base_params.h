#ifndef PHARE_TESTS_CORE_DATA_GRIDLAYOUT_GRIDLAYOUT_BASE_PARAMS_H
#define PHARE_TESTS_CORE_DATA_GRIDLAYOUT_GRIDLAYOUT_BASE_PARAMS_H

#include <memory>


#include "core/data/grid/gridlayout.h"
#include "core/data/grid/gridlayoutdefs.h"
#include "core/data/ndarray/ndarray_vector.h"
#include "gridlayout_utilities.h"
#include "core/utilities/point/point.h"

using namespace PHARE::core;

template<typename GridLayoutImpl>
struct GridLayoutTestParam
{
    std::shared_ptr<GridLayout<GridLayoutImpl>> layout;
    static constexpr std::size_t dim         = GridLayoutImpl::dimension;
    static constexpr std::size_t interpOrder = GridLayoutImpl::interp_order;

    std::array<double, dim> dxdydz;
    std::array<uint32, dim> nbCellXYZ;

    Point<double, dim> origin;

    HybridQuantity::Scalar currentQuantity;

    std::shared_ptr<Field<decltype(getNdArrayVecImpl(SelectorDim<dim>{})), HybridQuantity::Scalar>>
        field;


    template<typename Container, std::size_t... I>
    auto makeIt_(Container allocSize, std::index_sequence<I...>)
    {
        return std::make_shared<
            Field<decltype(getNdArrayVecImpl(SelectorDim<dim>{})), HybridQuantity::Scalar>>(
            "field", currentQuantity, (allocSize[I])...);
    }

    template<typename Container>
    auto makeMyField_(Container allocSize)
    {
        std::make_index_sequence<dim> idx;
        return makeIt_(allocSize, idx);
    }
};


#endif
