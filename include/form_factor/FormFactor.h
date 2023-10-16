#pragma once

#include <form_factor/FormFactorType.h>
#include <form_factor/FormFactor.h>
#include <form_factor/FormFactorTable.h>
#include <constants/ConstantsFwd.h>
#include <data/DataFwd.h>

#include <array>
#include <string_view>
#include <stdexcept>
#include <cmath>

namespace form_factor {
    class FormFactor {
        public:
            constexpr FormFactor(std::array<double, 5> a, std::array<double, 5> b, double c) : a(a), b(b), c(c) {
                initialize();
            }

        private: 
            std::array<double, 5> a;
            std::array<double, 5> b;
            double c;

            double f0 = 1;

            constexpr void initialize() {
                f0 = a[0] + a[1] + a[2] + a[3] + a[4] + c;
            }

        public: 
            /**
             * @brief Evaluate the form factor at a given q value.
             *        The form factor is normalized to 1 at q = 0.
             */
            constexpr double evaluate(double q) const {
                double sum = 0;
                for (unsigned int i = 0; i < 5; ++i) {
                    sum += a[i]*std::exp(-b[i]*q*q);
                }
                return (sum + c)/f0;
            }

            constexpr double constexpr_evaluate(double q) const noexcept {
                double sum = 0;
                for (unsigned int i = 0; i < 5; ++i) {
                    sum += a[i]*std::exp(-b[i]*q*q);
                }
                return (sum + c)/f0;
            }
    };

    /**
     * @brief Get the number of unique form factors.
     * 
     * This can be used to iterate over all form factors.
     */
    constexpr unsigned int get_count() {
        return static_cast<unsigned int>(form_factor_t::COUNT);
    }

    /**
     * @brief Get the number of unique form factors, excluding the excluded volume form factor.
     * 
     * This can be used to iterate over all form factors except the excluded volume form factor.
     */
    constexpr unsigned int get_count_without_excluded_volume() {
        return static_cast<unsigned int>(form_factor_t::COUNT)-1;
    }

    /**
     * @brief Get the form factor type based on an atom type. 
     *        In case the atom type is not recognized, the default form factor (argon) is returned.
     */
    constexpr form_factor_t get_type(constants::atom_t atom_type) {
        switch(atom_type) {
            case constants::atom_t::H: return form_factor_t::H;
            case constants::atom_t::C: return form_factor_t::C;
            case constants::atom_t::N: return form_factor_t::N;
            case constants::atom_t::O: return form_factor_t::O;
            default: return form_factor_t::OTHER;
        }
    }

    /**
     * @brief Get the form factor type based on an atom type and an atomic group.
     *        The atomic group takes priority. Only if the atomic group is not recognized, the atom type is used.
     *        In case either the atomic group or the atom type is not recognized, the default form factor (argon) is returned.
     */
    constexpr form_factor_t get_type(constants::atom_t atom_type, constants::atomic_group_t atomic_group) {
        switch(atomic_group) {
            case constants::atomic_group_t::CH: return form_factor_t::CH;
            case constants::atomic_group_t::CH2: return form_factor_t::CH2;
            case constants::atomic_group_t::CH3: return form_factor_t::CH3;
            case constants::atomic_group_t::NH: return form_factor_t::NH;
            case constants::atomic_group_t::NH2: return form_factor_t::NH2;
            case constants::atomic_group_t::OH: return form_factor_t::OH;
            case constants::atomic_group_t::SH: return form_factor_t::SH;
            default: return get_type(atom_type);
        }
    }

    /**
     * This struct contains the form factors of the most common atomic elements encountered in SAXS. 
     */
    namespace storage {
        // atomic
        inline constexpr FormFactor H               = FormFactor(               constants::form_factor::H::a,               constants::form_factor::H::b,               constants::form_factor::H::c);
        inline constexpr FormFactor C               = FormFactor(               constants::form_factor::C::a,               constants::form_factor::C::b,               constants::form_factor::C::c);
        inline constexpr FormFactor N               = FormFactor(               constants::form_factor::N::a,               constants::form_factor::N::b,               constants::form_factor::N::c);
        inline constexpr FormFactor O               = FormFactor(               constants::form_factor::O::a,               constants::form_factor::O::b,               constants::form_factor::O::c);
        inline constexpr FormFactor S               = FormFactor(               constants::form_factor::S::a,               constants::form_factor::S::b,               constants::form_factor::S::c);

        // atomic groups
        inline constexpr FormFactor CH_sp3          = FormFactor(          constants::form_factor::CH_sp3::a,          constants::form_factor::CH_sp3::b,          constants::form_factor::CH_sp3::c);
        inline constexpr FormFactor CH2_sp3         = FormFactor(         constants::form_factor::CH2_sp3::a,         constants::form_factor::CH2_sp3::b,         constants::form_factor::CH2_sp3::c);
        inline constexpr FormFactor CH3_sp3         = FormFactor(         constants::form_factor::CH3_sp3::a,         constants::form_factor::CH3_sp3::b,         constants::form_factor::CH3_sp3::c);
        inline constexpr FormFactor CH_sp2          = FormFactor(          constants::form_factor::CH_sp2::a,          constants::form_factor::CH_sp2::b,          constants::form_factor::CH_sp2::c);
        inline constexpr FormFactor CH_arom         = FormFactor(         constants::form_factor::CH_arom::a,         constants::form_factor::CH_arom::b,         constants::form_factor::CH_arom::c);
        inline constexpr FormFactor OH_alc          = FormFactor(          constants::form_factor::OH_alc::a,          constants::form_factor::OH_alc::b,          constants::form_factor::OH_alc::c);
        inline constexpr FormFactor OH_acid         = FormFactor(         constants::form_factor::OH_acid::a,         constants::form_factor::OH_acid::b,         constants::form_factor::OH_acid::c);
        inline constexpr FormFactor O_res           = FormFactor(           constants::form_factor::O_res::a,           constants::form_factor::O_res::b,           constants::form_factor::O_res::c);
        inline constexpr FormFactor NH              = FormFactor(              constants::form_factor::NH::a,              constants::form_factor::NH::b,              constants::form_factor::NH::c);
        inline constexpr FormFactor NH2             = FormFactor(             constants::form_factor::NH2::a,             constants::form_factor::NH2::b,             constants::form_factor::NH2::c);
        inline constexpr FormFactor NH_plus         = FormFactor(         constants::form_factor::NH_plus::a,         constants::form_factor::NH_plus::b,         constants::form_factor::NH_plus::c);
        inline constexpr FormFactor NH2_plus        = FormFactor(        constants::form_factor::NH2_plus::a,        constants::form_factor::NH2_plus::b,        constants::form_factor::NH2_plus::c);
        inline constexpr FormFactor NH3_plus        = FormFactor(        constants::form_factor::NH3_plus::a,        constants::form_factor::NH3_plus::b,        constants::form_factor::NH3_plus::c);
        inline constexpr FormFactor NH_guanine      = FormFactor(      constants::form_factor::NH_guanine::a,      constants::form_factor::NH_guanine::b,      constants::form_factor::NH_guanine::c);
        inline constexpr FormFactor NH2_guanine     = FormFactor(     constants::form_factor::NH2_guanine::a,     constants::form_factor::NH2_guanine::b,     constants::form_factor::NH2_guanine::c);
        inline constexpr FormFactor SH              = FormFactor(              constants::form_factor::SH::a,              constants::form_factor::SH::b,              constants::form_factor::SH::c);

        // average excluded volume
        inline constexpr FormFactor excluded_volume = FormFactor( constants::form_factor::excluded_volume::a, constants::form_factor::excluded_volume::b, constants::form_factor::excluded_volume::c);

        // all others; this is just the form factor of argon
        inline constexpr FormFactor other           = FormFactor(           constants::form_factor::other::a,           constants::form_factor::other::b,           constants::form_factor::other::c);

        constexpr const FormFactor& get_form_factor(form_factor_t type) {
            switch (type) {
                case form_factor_t::H:
                    return H;
                case form_factor_t::C:
                    return C;
                case form_factor_t::N:
                    return N;
                case form_factor_t::O:
                    return O;
                case form_factor_t::S:
                    return S;
                case form_factor_t::CH:
                    return CH_sp3;
                case form_factor_t::CH2:
                    return CH2_sp3;
                case form_factor_t::CH3:
                    return CH3_sp3;
                case form_factor_t::NH:
                    return NH;
                case form_factor_t::NH2:
                    return NH2;
                case form_factor_t::OH:
                    return OH_alc;
                case form_factor_t::SH:
                    return SH;
                case form_factor_t::OTHER:
                    return other;
                case form_factor_t::EXCLUDED_VOLUME:
                    return excluded_volume;
                default:
                    throw std::runtime_error("FormFactorStorage::get_form_factor: Invalid form factor type (enum " + std::to_string(static_cast<int>(type)) + ")");
            }
        }
    };
}