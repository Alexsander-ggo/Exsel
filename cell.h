#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class Sheet;

class Cell : public CellInterface {
public:
                                  Cell(Sheet& sheet, Position pos);

    void                          Set(std::string text);

    void                          Clear();

    Value                         GetValue() const override;

    std::string                   GetText() const override;

    std::vector<Position>         GetReferencedCells() const override;

    bool                          IsReferenced() const;

    bool                          IsEmpty() const;

private:
    class Impl {
    public:
        virtual                             ~Impl() = default;

        virtual Value                       GetValue() const = 0;

        virtual std::string                 GetText() const = 0;

        virtual std::vector<Position>       GetReferencedCells() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl()
            : Impl() {}

        Value GetValue() const override {
            return std::string();
        }

        std::string GetText() const override {
            return std::string();
        }

        std::vector<Position> GetReferencedCells() const override {
            return {};
        }
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string str)
            : Impl()
            , str_(std::move(str)) {}

        Value GetValue() const override {
            return (str_.front() != ESCAPE_SIGN) ? str_ : str_.substr(1);
        }

        std::string GetText() const override {
            return str_;
        }

        std::vector<Position> GetReferencedCells() const override {
            return {};
        }

    private:
        std::string str_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string str, const SheetInterface& sheet)
            : Impl()
            , sheet_(sheet)
            , formula_(std::move(ParseFormula(str))) {}

        Value GetValue() const override {
            auto res = formula_->Evaluate(sheet_);
            if (std::holds_alternative<double>(res)) {
                return std::get<double>(res);
            }
            return std::get<FormulaError>(res);
        }

        std::string GetText() const override {
            return FORMULA_SIGN + formula_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const override {
            return formula_->GetReferencedCells();
        }

    private:
        const SheetInterface& sheet_;
        std::unique_ptr<FormulaInterface> formula_;
    };

private:
    Sheet&                            sheet_;
    Position                          pos_;
    std::unique_ptr<Impl>             impl_;
    Positions                         dependents_;
    Positions                         effects_;
    mutable std::optional<Value>      cache_;

private:
    std::unique_ptr<Impl>             MakeImpl(std::string text) const;

    Cell*                             MakeCell(Position pos) const;

    bool                              IsCyclic(const Impl* impl) const;

    bool                              IsCyclicFormula(const Positions& dependents, Positions& checkeds) const;

    void                              UpdateCache() const;

    void                              InvalidCache();

    void                              RemoveOldDependents();

    void                              CreateNewDependents();

    void                              InvalidAllDependentCaches(const Positions& effects, Positions& invalids);

    void                              Init();
};