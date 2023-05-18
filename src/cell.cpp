#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <utility>

using namespace std::literals;


Cell::Value EmptyImpl::GetValue() const {
    return ""s;
}
std::string EmptyImpl::GetText() const {
    return ""s;
}

std::vector<Position> EmptyImpl::GetReferencedCells() const {
    return {};
}

TextImpl::TextImpl(std::string text) : impl_(std::move(text)) {

}

Cell::Value TextImpl::GetValue() const {
    if (impl_[0] == '\'') {
        return std::string(impl_.begin() + 1, impl_.end());
    }
    return impl_;
}


std::string TextImpl::GetText() const {
    return impl_;
}

std::vector<Position> TextImpl::GetReferencedCells() const {
    return {};
}

FormulaImpl::FormulaImpl(std::string formula, Sheet& sheet) : impl_(formula), sheet_(sheet) {

}

Cell::Value FormulaImpl::GetValue() const {
    try {
        auto result = impl_.Evaluate(sheet_);
        return std::get<double>(result);
    }
    catch (FormulaError& er) {
        return er;
    }

}

std::string FormulaImpl::GetText() const {
    using namespace std::literals;
    return FORMULA_SIGN + impl_.GetExpression();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return impl_.GetReferencedCells();
}


Cell::Cell(Sheet& sheet) : sheet_(sheet) {

}

Cell::~Cell() {

}

void Cell::Set(Position pos, std::string text) {
    pos_ = std::move(pos);
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == '=' && text[1] != '\'') {
        std::string expression(text.begin() + 1, text.end());
        impl_ = std::make_unique<FormulaImpl>(std::move(expression), sheet_);
    }
    else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

}

void Cell::Clear() {
    Set(pos_, ""s);
}

Cell::Value Cell::GetValue() const {
    return sheet_.GetCellCache(pos_);
}

std::string Cell::GetText() const {
    return impl_->GetText();

}

Cell::Value Cell::CalculateValue() const {
    return impl_->GetValue();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}
