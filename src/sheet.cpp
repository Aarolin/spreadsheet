#include "sheet.h"

#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::Sheet() {

}

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {

    CheckPosValidity(pos);
    int row_id = pos.row;
    int col_id = pos.col;


    if (cache_.count(pos) != 0) {
		const std::string& prev_expr = cache_.at(pos).first;
		if (text == prev_expr) {
			return;
		}
        
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(pos, text);
        auto referenced_cells = new_cell->GetReferencedCells();
        for (const Position& cell : referenced_cells) {
            if (HasCircularDependecies(pos, cell)) {
                throw CircularDependencyException("There is a circular dependency in this expression"s);
            }
        }
        
        try {
            auto old_dependencies = main_sheet_[row_id][col_id]->GetReferencedCells();
            auto cell_value = new_cell->CalculateValue();
            UpdateCache(pos, std::move(text), cell_value);
            CountDependentCells(pos);
            UpdateDependencies(old_dependencies, new_cell->GetReferencedCells(), pos);
            main_sheet_[row_id][col_id].reset(new_cell.release());
        }
        catch (FormulaError& er) {
            throw er;
        }
        return;
    }

    CreateNewCell(pos, text);
    
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosValidity(pos);
    if (main_sheet_.count(pos.row) == 0) {
        return nullptr;
    }
    auto& row = main_sheet_.at(pos.row);

    if (row.count(pos.col) == 0) {
        return nullptr;
    }

    return main_sheet_.at(pos.row).at(pos.col).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosValidity(pos);
    auto& result = main_sheet_[pos.row][pos.col];
    if (!result) {
        if (dependencies_.count(pos) != 0) {
            result = std::make_unique<Cell>(*this);
            result->Set(pos, "");
        }
    }
    return result.get();
}

void Sheet::ClearCell(Position pos) {
    CheckPosValidity(pos);
    int row_id = pos.row;
    int col_id = pos.col;
    if (main_sheet_.count(row_id) == 0) {
        return;
    }
    auto& row = main_sheet_[row_id];
    if (row.count(col_id) == 0) {
        return;
    }

    auto& cell_to_clear = main_sheet_[row_id][col_id];
    if (cell_to_clear) {
        cell_to_clear->Clear();
        InactivePosition(pos);
        cell_to_clear.release();
        row.erase(col_id);
    }
}

Size Sheet::GetPrintableSize() const {
    return print_area_;
}

void Sheet::PrintValues(std::ostream& output) const {
    int rows = print_area_.rows;
    int cols = print_area_.cols;
    if (rows < 1 || cols < 1) {
        return;
    }
    for (int i = 0; i < rows; ++i) {
        if (main_sheet_.count(i) == 0) {
            for (int j = 0; j < cols; ++j) {
                if (j != cols - 1) {
                    output << '\t';
                }
            }
            output << '\n';
            continue;
        }
        const auto& row = main_sheet_.at(i);
        for (int j = 0; j < cols; ++j) {
            if (row.count(j) != 0) {
                auto& cell = row.at(j);
                auto val = cell->GetValue();
                PrintValue(output, val);
            }
            if (j != cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
    
}

void Sheet::PrintTexts(std::ostream& output) const {
    int rows = print_area_.rows;
    int cols = print_area_.cols;

    if (rows < 1 || cols < 1) {
        return;
    }
    for (int i = 0; i < rows; ++i) {
        if (main_sheet_.count(i) == 0) {
            for (int j = 0; j < cols; ++j) {
                if (j != cols - 1) {
                    output << '\t';
                }
            }
            output << '\n';
            continue;
        }
        const auto& row = main_sheet_.at(i);
        for (int j = 0; j < cols; ++j) {
            if (row.count(j) != 0) {
                auto& val = row.at(j);
                output << val->GetText();
            }
            if (j != cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::CreateNewCell(Position pos, std::string text) {
    int row_id = pos.row;
    int col_id = pos.col;

    auto new_cell = std::make_unique<Cell>(*this);
    new_cell->Set(pos, text);
    auto referenced_cells = new_cell->GetReferencedCells();
    for (const Position& cell : referenced_cells) {
        if (HasCircularDependecies(pos, cell)) {
            throw CircularDependencyException("There is a circular dependency in this expression"s);
        }
    }

    try {
        auto cell_value = new_cell->CalculateValue();
        UpdateCache(pos, std::move(text), cell_value);
        AssignDependencies(pos, new_cell->GetReferencedCells());
        ActivePosition(pos);
        main_sheet_[row_id][col_id].reset(new_cell.release());
    }
    catch (FormulaError& er) {
        throw er;
    }
    return;
}

void Sheet::CheckPosValidity(Position pos) const {
    int row_id = pos.row;
    int col_id = pos.col;
    if (row_id < 0 || col_id < 0) {
        throw InvalidPositionException("Invalid id of row or column"s);
    }
    else if (row_id >= Position::MAX_ROWS || col_id >= Position::MAX_COLS) {
        throw InvalidPositionException("Invalid id of row or column");
    }
}

void Sheet::ActivePosition(Position pos) {
    if (max_row_ < pos.row) {
        max_row_ = pos.row;
        print_area_.rows = max_row_ + 1;
    }

    if (max_col_ < pos.col) {
        max_col_ = pos.col;
        print_area_.cols = max_col_ + 1;
    }

    active_cells_.insert(pos);

}

void Sheet::InactivePosition(Position pos) {

    active_cells_.erase(pos);
    max_row_ = -1;
    max_col_ = -1;
    print_area_ = GetActualTableArea();
}

Size Sheet::GetActualTableArea() {
    
    for (const auto& pos : active_cells_) {
        if (pos.row > max_row_) {
            max_row_ = pos.row;
        }
        if (pos.col > max_col_) {
            max_col_ = pos.col;
        }
    }
    return { max_row_ + 1, max_col_ + 1};
}

void Sheet::PrintValue(std::ostream& os, const CellInterface::Value& value) const {

    if (std::holds_alternative<double>(value)) {
        os << std::get<double>(value);
    }
    else if (std::holds_alternative<std::string>(value)) {
        os << std::get<std::string>(value);
    }
    else if (std::holds_alternative<FormulaError>(value)) {
        os << std::get<FormulaError>(value);
    }
}


bool Sheet::CellCacheIsExist(Position pos) const {
    return cache_.count(pos) == 0;
}


Sheet::CellValue Sheet::GetCellCache(Position pos) const {
    return cache_.at(pos).second;
}


bool Sheet::HasCircularDependecies(Position source_pos, Position ref_pos) const {

    if (source_pos == ref_pos) {
        return true;
    }

    if (dependencies_.count(source_pos) == 0) {
        return false;
    }

    return dependencies_.at(source_pos).count(ref_pos) != 0;
    return false;
}


void Sheet::UpdateCache(Position& pos, std::string text, const CellValue& new_value) {
    cache_[pos].second = new_value;
    cache_[pos].first = std::move(text);
}


void Sheet::AssignDependencies(Position& source_pos, const std::vector<Position>& incoming_positions) {
    for (const auto& income_pos : incoming_positions) {
        dependencies_[income_pos].insert(source_pos);
    }

    if (dependencies_.count(source_pos) != 0) {
		const auto& deep_dependencies = dependencies_.at(source_pos);
		for (const auto& deep_depend_pos : deep_dependencies) {
			for (const auto& income_pos : incoming_positions) {
				dependencies_[income_pos].insert(deep_depend_pos);
			}
		}
    }

}

void Sheet::UpdateDependencies(const std::vector<Position>& old_dependencies, const std::vector<Position>& new_dependecies, Position& pos) {
    //список €чеек на которые сейчас вли€ет €чейка с позицией pos
    auto& dependent_cells_list = dependencies_[pos];

    //¬ случае, изменили формулу в €чейке, то нам нужно сделать так, чтобы €чейки из старой формулы перестали вли€ть на текущую, и помен€ть их на новые.
    for (const auto& old_depend_cell : old_dependencies) {
        for (const auto& curr_depend_cell : dependent_cells_list) {
            dependencies_[old_depend_cell].erase(curr_depend_cell);
        }
        dependencies_[old_depend_cell].erase(pos);
    }

    for (const auto& new_depend_cell : new_dependecies) {
        for (const auto& curr_depend_cell : dependent_cells_list) {
            dependencies_[new_depend_cell].insert(curr_depend_cell);
        }
        dependencies_[new_depend_cell].insert(pos);
    }

}

void Sheet::CountDependentCells(const Position& pos) {

    if (dependencies_.count(pos) == 0) {
        return;
    }

    for (const auto& depenedent : dependencies_.at(pos)) {
        const auto& cell = main_sheet_.at(depenedent.row).at(depenedent.col);
        cache_.at(depenedent).second = cell->CalculateValue();
    }

}