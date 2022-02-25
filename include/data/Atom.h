#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "data/Record.h"
#include "Tools.h"
#include "constants.h"
#include "math/Vector3.h"
#include "Exceptions.h"

using std::vector, std::string, std::shared_ptr, std::unique_ptr;

class Atom : public Record {
  public:
    Atom(const Atom&& a) noexcept;

    Atom(const Atom& a);

    /** 
     * @brief Construct a new Atom object.
     * @param v a TVector3 containing the x, y, z coordinates of the atom. 
     * @param occupancy the occupancy of this atom.
     * @param element the atomic element of the base atom.
     * @param name the molecule (e.g. HOH).
     * @param serial the serial number of this atom.
     */
    Atom(const Vector3 v, const double occupancy, const string element, const string name, int serial);

    /**
     * @brief Construct a new Atom object.
     * @param all see http://www.wwpdb.org/documentation/file-format-content/format33/sect9.html#ATOM
     */
    Atom(const int serial, const string name, const string altLoc, const string resName, const string chainID, const int resSeq, 
        const string iCode, const Vector3 coords, const double occupancy, const double tempFactor, const string element, const string charge);

    /**
     * @brief Construct a new empty Atom object.
     */
    Atom();

    virtual ~Atom() override {}

    RecordType get_type() const override;

    /**
     * @brief Set the properties of this Atom based on a .pdb format string. 
     * @param s A .pdb format ATOM string. 
     */
    void parse_pdb(const string s) override;

    /**
     * @brief Create a .pdb format string representation of this Atom. 
     * @return The string representation of this Atom. 
     */
    string as_pdb() const override;

    /** 
     * @brief Calculate the distance to another atom. 
     */
    double distance(const Atom& a) const;

    /** 
     * @brief Prints the contents of this object to the terminal. (NOT FULLY IMPLEMENTED!)
     */
    void print() const;

    /** 
     * @brief Move this atom by a vector.
     * @param v the translation vector.
     */
    void translate(const Vector3 v);

    /**
     * @brief Determine if this is a water molecule. Only used by the Hetatom subclass, but is defined here for convenience. 
     * @return true if this is a water molecule, otherwise false. 
     */
    virtual bool is_water() const;

//*** setters ***//
    void set_coordinates(const Vector3 v);
    void set_x(double x);
    void set_y(double y);
    void set_z(double z);
    void set_occupancy(double occupancy);
    void set_tempFactor(double tempFactor);
    void set_altLoc(string altLoc);
    void set_serial(int serial);
    void set_resSeq(int resSeq);
    void set_effective_charge(double charge);
    void set_chainID(string chainID);
    void set_iCode(string iCode);
    void set_charge(string charge);

    /**
     * @brief Set the residue name for this atom.
     * @param resName the residue name, typically an amino acid such as LYS.
     */
    void set_resName(string resName);

    /**
     * @brief Specify the position of this atom within its residue.
     * @param name the position specifier, e.g. CG2 (Carbon | position G | branch 2).
     */
    void set_name(string name);

    /**
     * @brief Set the atomic element for this atom. Any spaces are removed. 
     * @param element the atomic element, e.g. He.
     */
    void set_element(string element);

//*** getters ***//
    Vector3& get_coordinates();
    const Vector3& get_coordinates() const;
    int get_serial() const;
    int get_resSeq() const;
    double get_occupancy() const;
    double get_tempFactor() const;
    double get_effective_charge() const;
    string get_altLoc() const;
    string get_chainID() const;
    string get_iCode() const;
    string get_charge() const;
    string get_resName() const;
    string get_name() const;
    string get_element() const;
    virtual string get_recName() const;

    double get_mass() const;

    /**
     * @brief Add @p charge to the effective charge of this atom. 
     */
    void add_effective_charge(const double charge) {effective_charge += charge;}

    /**
     * @brief Comparison function to allow this class to be a map key. 
     * @param rhs Atom to compare against.
     */
    bool operator<(const Atom& rhs) const;

    /**
     * @brief Equality operator to determine if two atoms are equal.
     *        Note that this compares their unique object identifier which is generated at object creation, completely disregarding
     *        their contents. Unless a deliberate attempt at desyncing the id from the contents were made, equality of content follows
     *        from equality of id. 
     * @param rhs Atom to compare against. 
     */
    bool operator==(const Atom& rhs) const;

    /**
     * @brief Equality operator to determine if two atoms are equal.
     *        Note that this is a @a content comparator, and thus determines if two atoms are equal based on their contents. 
     * @param rhs Atom to compare against. 
     */
    bool equals_content(const Atom& rhs) const;

    /**
     * @brief Equality operator to determine if two atoms are equal.
     *        Note that this compares their unique object identifier which is generated at object creation, completely disregarding
     *        their contents. Unless a deliberate attempt at desyncing the id from the contents were made, equality of content follows
     *        from equality of id. 
     * @param rhs Atom to compare against. 
     */
    bool equals(const Atom& rhs) const;

    /**
     * @brief Inequality operator to determine if two atoms are not equal.
     *        Note that this compares their unique object identifier which is generated at object creation, completely disregarding
     *        their contents. Unless a deliberate attempt at desyncing the id from the contents were made, inequality of content follows
     *        from inequality of id. 
     * @param rhs Atom to compare against. 
     */
    bool operator!=(const Atom& rhs) const {return !operator==(rhs);}

    Atom& operator=(const Atom& rhs);

    // properties as defined in https://ftp.wwpdb.org/pub/pdb/doc/format_descriptions/Format_v33_A4.pdf, page 180.
    Vector3 coords = {0, 0, 0};
    string name = "", altLoc = "", resName = "", chainID = "", iCode = "", element = "", charge = "";
    double occupancy = -1, tempFactor = -1;
    int serial = -1, resSeq = -1; 

    // other properties
    double effective_charge = -1;
    int uid = -1;

    // global counter for unique ids
    static inline int uid_counter = 0;
};