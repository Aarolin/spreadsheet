#pragma once

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <map>
#include <unordered_set>
#include <utility>

class Cell;

class Sheet : public SheetInterface {
public:
    using CellValue = CellInterface::Value;

    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    bool CellCacheIsExist(Position pos) const;
    CellValue GetCellCache(Position pos) const;

    bool HasCircularDependecies(Position source_pos, Position ref_pos) const;
    

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<int, std::unordered_map<int, std::unique_ptr<Cell>>> main_sheet_;
    std::unordered_map<Position, std::pair<std::string, CellValue>, PositionHasher, PostionEqual> cache_;
    std::unordered_map<Position, std::unordered_set<Position, PositionHasher, PostionEqual>, PositionHasher, PostionEqual> dependencies_;
    std::unordered_set<Position, PositionHasher, PostionEqual> active_cells_;

    Size print_area_ = { 0, 0 };
    int max_row_ = -1;
    int max_col_ = -1;
    
    void CreateNewCell(Position pos, std::string text);

    void CheckPosValidity(Position pos) const;

    void ActivePosition(Position pos);
    void InactivePosition(Position pos);

    Size GetActualTableArea();
    void PrintValue(std::ostream& os, const CellInterface::Value& value) const;

    void UpdateCache(Position& pos, std::string text, const CellValue& new_value);
    void AssignDependencies(Position& source_pos, const std::vector<Position>& dependent_pos);
    void UpdateDependencies(const std::vector<Position>& old_dependencies, const std::vector<Position>& new_dependecies, Position& pos);
    void CountDependentCells(const Position& pos);
};


