#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Impl {
public:
    using Value = CellInterface::Value;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual ~Impl() = default;
};


class EmptyImpl : public Impl {
public:
    EmptyImpl() = default;
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
};

class TextImpl : public Impl {
public:
    explicit TextImpl(std::string txt);
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
private:
    std::string impl_;
};

class FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string, Sheet& sheet);
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
private:
    Formula impl_;
    Sheet& sheet_;
};


class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(Position pos, std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    Value CalculateValue() const;

private:
    
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    Position pos_;

};

    // Добавьте поля и методы для связи с таблицей, проверки циклических 
    // зависимостей, графа зависимостей и т. д.