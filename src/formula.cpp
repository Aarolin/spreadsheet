#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <utility>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    FormulaError::Category er_category = fe.GetCategory();
    if (er_category == FormulaError::Category::Div0) {
        return output << "#DIV/0!";
    }
    else if (er_category == FormulaError::Category::Value) {
        return output << "#VALUE!";
    }
    return output << "#REF!";
}

Formula::Formula(std::string expr) try : ast_(ParseFormulaAST(std::move(expr))) {

} catch (...) {
    using namespace std::literals;
    throw FormulaException("Incorrect expression"s);
}

FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
    auto values_to_referenced_cells = GetValuesOfReferencedCells(sheet);
    return ast_.Execute(values_to_referenced_cells);
}

std::string Formula::GetExpression() const {
    std::ostringstream os;
    ast_.PrintFormula(os);
    return os.str();
}


std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}


std::unordered_map<Position, double, PositionHasher> Formula::GetValuesOfReferencedCells(const SheetInterface& sheet) const {

    std::unordered_map<Position, double, PositionHasher> result;
    const auto& incoming_cells = ast_.GetCells();

    for (const auto& cell_pos : incoming_cells) {
        const auto cell = sheet.GetCell(cell_pos);
        if (!cell) {
            result.insert({ cell_pos, 0 });
            continue;
        }
        auto cell_value = cell->GetValue();
        if (std::holds_alternative<double>(cell_value)) {
            result.insert({ cell_pos, std::get<double>(cell_value) });
            continue;
        }
        else if (std::holds_alternative<std::string>(cell_value)) {
            std::string str_value = std::get<std::string>(cell_value);
            try {
                if (!IsValidStr(str_value)) {
                    throw std::invalid_argument("Can't transform str to double");
                }
                double num_str_val = std::stod(str_value);
                result.insert({cell_pos, num_str_val});
            }
            catch (std::invalid_argument&) {
                if (str_value.empty()) {
                    result.insert({ cell_pos, 0 });
                    continue; 
                }
                throw FormulaError(FormulaError::Category::Value);
            }
            catch (std::out_of_range&) {
                throw FormulaError(FormulaError::Category::Value);
            }
            continue;
        }
        auto error = std::get<FormulaError>(cell_value);
        throw FormulaError(error.GetCategory());
    }

    return result;
}

std::vector<Position> Formula::GetReferencedCells() const {
    const auto& referenced_cells = ast_.GetCells();
    std::set<Position> unique_cells{ referenced_cells.begin(), referenced_cells.end() };
    return { unique_cells.begin(), unique_cells.end() };
}


bool IsValidStr(const std::string& str) {
    for (const char& ch : str) {
        if (!std::isdigit(ch) && ch != '-' && ch != '+' && ch != 'e' && ch != 'E' && ch != '.') {
            return false;
        }
    }
    return true;
}