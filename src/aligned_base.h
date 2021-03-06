/*
Copyright (c) 2006-2017 Elmar Pruesse <elmar.pruesse@ucdenver.edu>

This file is part of SINA.
123456789012345678901234567890123456789012345678901234567890123456789012
SINA is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

SINA is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with SINA.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify SINA, or any covered work, by linking or combining it
with components of ARB (or a modified version of that software),
containing parts covered by the terms of the
ARB-public-library-license, the licensors of SINA grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code
for the parts of ARB used as well as that of the covered work.
*/

#ifndef _ALIGNED_BASE_H_
#define _ALIGNED_BASE_H_

#include <limits>
#include <iostream>
#include <exception>

namespace sina {

class base_iupac {
public:
    typedef unsigned char value_type;
    static const value_type iupac_char_to_bmask[256];
    static const unsigned char bmask_to_iupac_char[32];
    static const float base_pairings[256];

    class bad_character_exception : public std::exception {
    public:
        bad_character_exception(value_type c) throw()
            : character(c)
        {
        }
        virtual const char* what() const throw() {
            return "Character not IUPAC encoded base or gap";
        }
        value_type character;
    };


    enum base_types {
        BASE_A=0,
        BASE_G=1,
        BASE_C=2,
        BASE_TU=3,
        BASE_MAX=4,
        BASE_LC=4
    };
    enum base_types_bitmask {
        BASEM_A=1<<BASE_A,
        BASEM_G=1<<BASE_G,
        BASEM_C=1<<BASE_C,
        BASEM_TU=1<<BASE_TU,
        BASEM_LC=1<<BASE_LC
    };

    base_iupac(unsigned char c) 
      : _data(iupac_char_to_bmask[c]) 
    {
      if (_data == 0 && c != '-' && c != '.') 
        throw bad_character_exception(c);
    }
    base_iupac() : _data(0) 
    {
    }

    operator unsigned char() const { 
        return bmask_to_iupac_char[_data]; 
    }

    base_iupac& operator=(unsigned char c) {
        _data = iupac_char_to_bmask[c];
        if (_data == 0 && c != '-' && c != '.') 
            throw bad_character_exception(c);
        return *this;
    }

    void complement() {
        _data = ((_data & BASEM_G) << (BASE_C - BASE_G)) |
            ((_data & BASEM_C) >> (BASE_C - BASE_G)) |
            ((_data & BASEM_A) << (BASE_TU - BASE_A)) |
            ((_data & BASEM_TU) >> (BASE_TU - BASE_A)) |
            (_data & BASEM_LC);
    }

    void setLowerCase() {
        _data |= BASEM_LC;
    }
    void setUpperCase() {
        _data &= ~BASEM_LC;
    }
    bool isLowerCase() const {
        return _data & BASEM_LC;
    }




    int ambig_order() const {
      return count_bits(_data & 0xf);
    }

    bool is_ambig() const {
      return count_bits(_data & 0xf) > 1;
    }

    base_types getBaseType() {
        // only valid if isAmbig==false
        const unsigned int t = 0x3000201;
        return static_cast<base_types>((t >> (_data & 0xF)) & 0xF);
    }

    bool has_A()  const { return _data & BASEM_A;  }
    bool has_G()  const { return _data & BASEM_G;  }
    bool has_C()  const { return _data & BASEM_C;  }
    bool has_TU() const { return _data & BASEM_TU; }


    bool comp(const base_iupac& rhs) const{
      //optimistic, match if IUPAC suggests match possible
      return 0xf & _data & rhs._data;

      //this would compute average
      //return 1.f - (2.f/count_bits(_data | rhs._data)) *
      //              count_bits(_data & rhs._data) ;
    }


    bool comp_pessimistic(const base_iupac& rhs) const {
      return !is_ambig() && (0xf & _data) == (0xf & rhs._data); 
    }

    typedef struct {
      float v[BASE_MAX*BASE_MAX];
    } matrix_type;

    // this does an IUPAC aware comparison using the given scoring matrix
    float comp(const base_iupac& rhs, const matrix_type& m) const {
      float rval = 0;
      int c = 0;

      // use some mean bit magic to do a real fast
      // log2(x) with x in 0,1,2,4
      const unsigned int t = 0x30002010;
      // given this array we can compute log2(x) as follows:
      // log2(x) = (t >> (x*4)) & 0xF
      // (shift x nibbles to right, mask everything but leftmost nibble)

      // "a &= a-1" unsets least significant bit
      // "a & -a" unsets all but least significant bit

      for(value_type lm = _data & 0xf; lm; lm &= lm-1) {
        unsigned char l = (t >> (((lm & -lm)-1)*4)) & 0xF;
        for (value_type rm = rhs._data & 0xf; rm; rm &= rm-1) {
          unsigned char r = (t >> (((rm &-rm)-1)*4)) & 0xF;
          rval += m.v[l*BASE_MAX+r];
          c++;
        }
      }

#if 0 // enable to verify above code
      float tval = 0;
      for (int l=0; l<BASE_MAX; l++) {
        if (_data & 1<<l) {
          for(int r=0; r<BASE_MAX; r++) {
            if (rhs._data & 1<<r) {
              tval += m.v[l*BASE_MAX+r];
            }
          }
        }
      }
      if (rval != tval) {
        std::cerr << "x12: " << rval << " " <<tval<<std::endl;
      }
#endif

      return rval/c;
    }

    float pair(const base_iupac& rhs) const {
        return pair(rhs, base_pairings);
    }

protected:
    int count_bits(unsigned char c) const {
      int rval = 0;
      while (c) {
        c &= c-1; // this will unset the least significant bit
        rval++;
      }
      return rval;
    }

    float pair(const base_iupac& rhs, const float *bp) const {
        return bp[((_data&0xf)<<4)+(rhs._data&0xf)];
    }

private:
    value_type _data;
};

template<typename T>
class aligned : public T
{
public:
    typedef unsigned int idx_type;
    typedef T base_type;

    aligned(idx_type pos=0, base_type base='-')
        : T(base), _idx(pos) {}

    base_type getBase() const { return *this;}
    void setBase(const T& b) { T::operator=(b); }

    idx_type getPosition() const { return _idx;}
    void setPosition(idx_type i) { _idx=i; }

    bool operator<(const aligned<T> &rhs) const {
      return _idx < rhs._idx;
    }

    float getWeight() const { return 1; }

private:
    idx_type _idx;
    friend struct aligned_base_reverse_position;
};

struct aligned_base_reverse_position {
    unsigned int pos;
    aligned_base_reverse_position(unsigned int p) : pos(p) {}
    template<typename T>
    void operator() (aligned<T>& ab) {
        ab._idx=pos-ab._idx;
    }
};

typedef aligned<base_iupac> aligned_base;
}// namespace sina

namespace std {
#if 0 //unused code
template<>
struct numeric_limits<sina::base> : numeric_limits<sina::base::value_type> {};
#endif
template<>
struct numeric_limits<sina::base_iupac> : numeric_limits<sina::base_iupac::value_type> {};
}


std::ostream& operator<<(std::ostream& out, sina::aligned_base ab);

#endif // _ALIGNED_BASE_H_

#if 0 //unused code
class base {
public:
    typedef unsigned char value_type;

    base(unsigned char c) : _data(c) {}
    base() : _data(0) {}

    operator unsigned char() const { return _data; }
    base& operator=(unsigned char c) { _data = c; return *this;}
    float comp(const base& rhs) { return (_data==rhs._data)?-1.0f:1.0f; }

    void complement() {
        switch (_data) {
            case 'A':
            case 'a':
                _data = 'T';
                break;
            case 'G':
            case 'g':
                _data = 'C';
                break;
            case 'C':
            case 'c':
                _data = 'G';
                break;
            case 'T':
            case 't':
            case 'U':
            case 'u':
                _data = 'A';
                break;
            default:
                _data = 'N';
        }
    }

private:
    value_type _data;
};
#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
