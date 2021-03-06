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

#include "query_arb.h"
#include "aligned_base.h"

#include <vector>
#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/range/iterator_range.hpp>

using sina::cseq;
using sina::query_arb;
using sina::aligned_base;
using boost::algorithm::first_finder;
using boost::algorithm::find_iterator;
using boost::algorithm::make_find_iterator;
using boost::algorithm::equals;
using std::cerr;
using std::endl;

// predicate checking aligned_base objects for possible match
// returns try if match is conceivable: N matches all
struct iupac_compare {
  typedef bool result_type;
  bool operator()(const aligned_base& a, const aligned_base& b) const {
    return a.comp(b);
  }
};

int main(int argc, const char **argv) {
  const char *arbdb = 0;
  const char *probe_seq = 0;
  bool reverse = false;
  bool complement = false;
  for (int i=1; i<argc; i++) {
    if (equals(argv[i], "--reverse")) {
      reverse = true;
    } else if (equals(argv[i], "--complement")) {
      complement = true;
    } else {
      if (!arbdb) {
	arbdb = argv[i];
      } else {
	probe_seq = argv[i];
      }
    }
  }

  query_arb *arb = query_arb::getARBDB(arbdb);

  // make set of fields to load
  std::vector<std::string> fields;
  fields.push_back("acc");
  fields.push_back("start");
  fields.push_back("stop");
  // cache sequences and meta data
  arb->loadCache(fields);
  // get sequences
  std::vector<cseq*> sequences = arb->getCacheContents();

  // turn probe into cseq
  cseq probe("probe");
  probe.append(probe_seq);
  if (reverse) {
    probe.reverse();
  }
  if (complement) {
    probe.complement();
  }

  // iterate over sequences
  for (std::vector<cseq*>::iterator it = sequences.begin();
       it != sequences.end(); it++) {

    // iterate over matches of probe
    typedef find_iterator<cseq::iterator> cseq_find_iterator;
    for (cseq_find_iterator jt =
	   make_find_iterator(**it, first_finder(probe, iupac_compare()));
	 jt != cseq_find_iterator(); ++jt) {

      // count ambiguities in match
      int iupac_count = 0;
      for (cseq::iterator kt = jt->begin(); kt != jt->end(); ++kt) {
	if (kt->is_ambig()) iupac_count ++;
      }

      // print match
      std::cout
	<< (*it)->get_attr<std::string>("acc") << "\t"
	<< (*it)->get_attr<int>("start") << "\t"
	<< (*it)->get_attr<int>("stop") << "\t"
	<< jt->begin()->getPosition() << "\t"
	<< jt->end()->getPosition() << "\t"
	<< iupac_count
	<< endl;
    }
  }
}
