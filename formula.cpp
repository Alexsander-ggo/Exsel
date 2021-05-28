#include "formula.h"

#include "FormulaAST.h"

#include <sstream>
#include <utility>

namespace {

    // -------------------Formula------------------------

    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
            : ast_(ParseFormulaAST(std::move(expression))) {}

        Value Evaluate(const SheetInterface& sheet) const override {
            CellLookup cell_lookup = [&sheet](Position pos) {
                auto value = sheet.GetCell(pos)->GetValue();
                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                }
                else if (std::holds_alternative<std::string>(value)) {
                    std::string text = sheet.GetCell(pos)->GetText();
                    if (text.empty()) {
                        return 0.0;
                    }
                    if (text.front() == ESCAPE_SIGN) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                    try {
                        return std::stod(text);
                    }
                    catch (...) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
                else {
                    throw std::get<FormulaError>(value);
                }
            };
            try {
                return ast_.Execute(cell_lookup);
            }
            catch (const FormulaError& fe) {
                return fe;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream os;
            ast_.PrintFormula(os);
            return os.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            auto positions_ = ast_.GetCells();
            return { positions_.begin(), positions_.end() };
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

// -------------------ParseFormula------------------------

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}