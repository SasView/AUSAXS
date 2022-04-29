#pragma once

// includes
#include <string>
#include <vector>
#include <data/Record.h>

class Footer : Record {
  public: 
    Footer() : contents("") {}

    Footer(const Footer& footer) noexcept : contents(footer.contents) {}

    Footer(const Footer&& footer) noexcept : contents(std::move(footer.contents)) {}

    ~Footer() override {}

    /**
     * @brief Get the RecordType of this object.
     * @return Record::Footer
     */
    RecordType get_type() const override {return FOOTER;}

    /**
     * @brief Parse a .pdb format Footer string. This is equivalent to the add method.
     * @param s the .pdb format Footer string.
     */
    void parse_pdb(const std::string s) override {add(s);}

    /**
     * @brief Get the .pdb format representation of this Footer. This is equivalent to the get method.
     * @return the .pdb format Footer string. 
     */
    std::string as_pdb() const override {return get();}

    /**
     * @brief Add a Footer line to the internal storage of this Footer. 
     * @param s the Footer line. 
     */
    void add(const std::string s) {contents += s + "\n";}

    /**
     * @brief Get the .pdb format representation of this Footer.
     * @return the .pdb format Footer string. 
     */
    std::string get() const {return contents;};

    Footer& operator=(const Footer& footer) {
      contents = footer.contents;
      return *this;
    }

  private: 
    std::string contents;
};