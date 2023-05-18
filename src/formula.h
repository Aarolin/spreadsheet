#pragma once

#include "common.h"
#include "FormulaAST.h"

#include <memory>
#include <vector>

// �������, ����������� ��������� � ��������� �������������� ���������.
// �������������� �����������:
// * ������� �������� �������� � �����, ������: 1+2*3, 2.5*(2+3.5/7)
// * �������� ����� � �������� ����������: A1+B2*C3
// ������, ��������� � �������, ����� ���� ��� ���������, ��� � �������. ���� ���
// �����, �� �� ������������ �����, ����� ��� ����� ���������� ��� �����. ������
// ������ ��� ������ � ������ ������� ���������� ��� ����� ����.
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // �������� ��������, ��� � ����� Evaluate() ������ �� ������� ��������� 
    // � �������� ���������.
    // ���������� ����������� �������� ������� ��� ����������� ����� ���� ������.
    // ���� ���������� �����-�� �� ��������� � ������� ����� �������� � ������, ��
    // ������������ ������ ��� ������. ���� ����� ������ ���������, ������������
    // �����.
    virtual Value Evaluate(const SheetInterface& sheet) const = 0;

    // ���������� ���������, ������� ��������� �������.
    // �� �������� �������� � ������ ������.
    virtual std::string GetExpression() const = 0;

    // ���������� ������ �����, ������� ��������������� ������������� � ����������
    // �������. ������ ������������ �� ����������� � �� �������� �������������
    // �����.
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

// ������ ���������� ��������� � ���������� ������ �������.
// ������� FormulaException � ������, ���� ������� ������������� �����������.
std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);

class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression);

    Value Evaluate(const SheetInterface& sheet) const override;

    std::string GetExpression() const override;

    std::vector<Position> GetReferencedCells() const override;

private:
    FormulaAST ast_;
    std::unordered_map<Position, double, PositionHasher> GetValuesOfReferencedCells(const SheetInterface& sheet) const;
};

bool IsValidStr(const std::string& str);