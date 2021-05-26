#pragma once

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

// -------------------Position------------------------

struct Position {
    int row = 0;
    int col = 0;

    bool                      operator==(Position rhs) const;

    bool                      operator<(Position rhs) const;

    bool                      IsValid() const;

    std::string               ToString() const;

    static Position           FromString(std::string_view str);

    static const int          MAX_ROWS = 16384;
    static const int          MAX_COLS = 16384;
    static const Position     NONE;
};

// -------------------HashPosition------------------------

class HashPosition {
public:
    size_t                      operator() (const Position& pos) const;

private:
    std::hash<int>              i_hasher;
};

// -------------------Positions------------------------

using Positions = std::unordered_set<Position, HashPosition>;

// -------------------Size------------------------

struct Size {
    int          rows = 0;
    int          cols = 0;

    bool         operator==(Size rhs) const;
};

// -------------------FormulaError------------------------

class FormulaError {
public:
    enum class Category {
        Ref,    
        Value,  
        Div0
    };

                               FormulaError(Category category);

    Category                   GetCategory() const;

    bool                       operator==(FormulaError rhs) const;

    std::string_view           ToString() const;

private:
    Category                   category_;
};

std::ostream& operator<<(std::ostream& output, FormulaError fe);

// -------------------InvalidPositionException------------------------

class InvalidPositionException : public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

// -------------------FormulaException------------------------

class FormulaException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// -------------------CircularDependencyException------------------------

class CircularDependencyException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// -------------------CellInterface------------------------

class CellInterface {
public:
    using Value = std::variant<std::string, double, FormulaError>;

    virtual                           ~CellInterface() = default;

    virtual Value                     GetValue() const = 0;

    virtual std::string               GetText() const = 0;

    virtual std::vector<Position>     GetReferencedCells() const = 0;
};

// -------------------CONST------------------------

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';

// -------------------SheetInterface------------------------

class SheetInterface {
public:
    virtual                                  ~SheetInterface() = default;

    virtual void                             SetCell(Position pos, std::string text) = 0;

    virtual const CellInterface*             GetCell(Position pos) const = 0;

    virtual CellInterface*                   GetCell(Position pos) = 0;

    virtual void                             ClearCell(Position pos) = 0;

    virtual Size                             GetPrintableSize() const = 0;

    virtual void                             PrintValues(std::ostream& output) const = 0;

    virtual void                             PrintTexts(std::ostream& output) const = 0;
};

// -------------------CreateSheet------------------------

std::unique_ptr<SheetInterface> CreateSheet();