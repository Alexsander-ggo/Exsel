#include "cell.h"

#include <iostream>
#include <sstream>

Cell::Cell(Sheet& sheet, Position pos)
    : CellInterface()
    , sheet_(sheet)
    , pos_(pos)
    , impl_(std::make_unique<EmptyImpl>()) {}

void Cell::Set(std::string text) {
    using namespace std::literals;
    auto tmp = MakeImpl(text);
    if (IsCyclic(tmp.get())) {
        throw CircularDependencyException("This formula is cyclical"s);
    }
    impl_ = std::move(tmp);
    Init();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    UpdateCache();
    return cache_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependents_.empty() || !effects_.empty();
}

bool Cell::IsEmpty() const {
    return impl_->GetText().empty();
}

/// передается копия text, далее эта копия никак не переносится, не изменяется, вроде как получается бесполезная
std::unique_ptr<Cell::Impl> Cell::MakeImpl(std::string text) const {
    using namespace std::literals;
    if (text.empty()) {
        return std::make_unique<EmptyImpl>();
    }
    if (text.front() == FORMULA_SIGN && text.size() > 1) {
        try {
            return std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        } catch (...) {
            throw FormulaException("Sytnax error"s);
        }
    }
    else {
        return std::make_unique<TextImpl>(text);
    }
}

Cell* Cell::MakeCell(Position pos) const {
    Cell* cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
    if (!cell) {
        sheet_.SetCell(pos, std::string());
        cell = MakeCell(pos);
    }
    return cell;
}

bool Cell::IsCyclic(const Impl* impl) const {
    auto positions = impl->GetReferencedCells();
    Positions dependents(positions.begin(), positions.end());
    Positions checkeds;
    return IsCyclicFormula(dependents, checkeds);
}

/// вроде как метод не использует поля классов и другие методы, думаю можно его убрать из класса, переделав в простую статическую функцию
bool Cell::IsCyclicFormula(const Positions& dependents, Positions& checkeds) const {
    if (dependents.count(pos_) != 0) {
        return true;
    }

    for (Position pos : dependents) {
        if (!pos.IsValid() || checkeds.count(pos) != 0) {
            continue;
        }
        checkeds.insert(pos);
        Cell* cell = MakeCell(pos);
        if (IsCyclicFormula(cell->dependents_, checkeds)) {
            return true;
        }
    }
    return false;
}

void Cell::UpdateCache() const {
    if (!cache_.has_value()) {
        cache_ = impl_->GetValue();
    }
}

void Cell::InvalidCache() {
    cache_ = std::nullopt;
}

void Cell::RemoveOldDependents() {
    for (Position pos : dependents_) {
        if (!pos.IsValid()) {
            continue;
        }
        Cell* cell = MakeCell(pos);
        cell->effects_.erase(pos);
    }
    dependents_.clear();
}

void Cell::CreateNewDependents() {
    for (Position pos : GetReferencedCells()) {
        if (!pos.IsValid()) {
            continue;
        }
        dependents_.insert(pos);
        Cell* cell = MakeCell(pos);
        cell->effects_.insert(pos);
    }
}

/// вроде как метод не использует поля классов и другие методы, думаю можно его убрать из класса, переделав в простую статическую функцию
void Cell::InvalidAllDependentCaches(const Positions& effects, Positions& invalids) {
    for (Position pos : effects) {
        if (!pos.IsValid()) {
            continue;
        }
        Cell* cell = MakeCell(pos);
        cell->InvalidCache();
        invalids.insert(pos);
        cell->InvalidAllDependentCaches(cell->effects_, invalids);
    }
}

void Cell::Init() {
    InvalidCache();
    RemoveOldDependents();
    CreateNewDependents();
    Positions invalids;
    InvalidAllDependentCaches(effects_, invalids);
}