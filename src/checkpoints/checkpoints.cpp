// Copyright (c) 2017-2018, The Mynt Project
// Portions Copyright (c) 2014-2017, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "include_base_utils.h"

using namespace epee;

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "include_base_utils.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include <sstream>
#include <random>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map< uint64_t, crypto::hash >::const_iterator highest = 
        std::max_element( m_points.begin(), m_points.end(),
                         ( boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _1) < 
                           boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _2 ) ) );
    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(bool testnet)
  {
    if (testnet)
    {
      ADD_CHECKPOINT(0,     "48ca7cd3c8de5b6a4d53d2861fbdaedca141553559f9be9520068053cda8430b");
      ADD_CHECKPOINT(1000000, "46b690b710a07ea051bc4a6b6842ac37be691089c0f7758cfeec4d5fc0b4a258");
      return true;
    }
    ADD_CHECKPOINT(1, "aeeff201e743104731977b820d40a8d95b1f07a4472c019e603271d0cb0f047a");
    ADD_CHECKPOINT(100, "d33ee17506e4bf8e588c59717baaf8233ffe6f0783e93e5b95d9f13dde326a61");
    ADD_CHECKPOINT(1000, "7656bae1cfede8dc962a603849e210736d1366a97ed3f6543b223160a2568f75");
    ADD_CHECKPOINT(1200, "12a70b65ff083e68a0260724596208367be88420089b546df07a510fa5978fc3");
    ADD_CHECKPOINT(1300, "ed0e00d6b456ba3c8ae06488f84e3e04a85b660564cfbd5e1dcb9c4e53714bd5");
    ADD_CHECKPOINT(1327, "d5d6c64d78e48f2ede80d6efcca81cc00b4e0bce4f7ddc376e3b936d3b2d63a1");
    ADD_CHECKPOINT(4577, "7d37a5d91270cb04294bdb6e09a6ce25131f68441d1307c7f3ec97c978f66bfa");
    ADD_CHECKPOINT(6014, "5ec2fef6918247e6b6cd17a256b379e60c2af67058a15d7d808d4774b5c7f6ad");
    ADD_CHECKPOINT(6112, "9d47805b39787cb4dfac49215f6a1729a39bc4dd8996a81b84598fe0aecaece4");
    ADD_CHECKPOINT(6368, "3129cf2d5cc8792aefaa5c2a9f77986a384bfb73381cb14d9dc6867cd1df9d92");
    ADD_CHECKPOINT(6556, "8a6466d57f1e95b543d14ac41783a94a83612b974303e5640b8da6c14d85d3bc");
    ADD_CHECKPOINT(7205, "3abe25b8ff8abaa2c628f79e73daff4d1c9ff7afb054ef8038717d4d346807f3");
    ADD_CHECKPOINT(8281, "a92baf6aaa903c62696d63f2f193000b482c6e63f9d58bdc0316d2c29ea87289");
    ADD_CHECKPOINT(8305, "fca0a4eaadab69324d948eb3086509de72ceefb861d28a27e983b089cdfc9add");
    ADD_CHECKPOINT(9109, "0860093f1cb43fd08e84fcad1a74b07746b2c810c7db81aa2692496a7b3d9507");
    ADD_CHECKPOINT(10689, "0f148437a494dc05a8402d150b6a6ebeffcf0a2d176de64349137a51566d4456");
    ADD_CHECKPOINT(11415, "62e45e4c5236dd547fd156600d4f41aefc8155d6bb98121cbb506b878257b246");
    ADD_CHECKPOINT(12029, "77254e38b4b16e0d65ed7cc9d2a6c2a557b9c1e20a9020c349cb10774bad4696");
    ADD_CHECKPOINT(12386, "7b485d5c82555d73dddbae201486b78fe1b4270ff0e52ac43d755189f6b8726d");
    ADD_CHECKPOINT(19700, "911d7d3d5a8d95eacb9cc2eed1f9edc78d9be2ed2f52f1a096e31d91cb11f9ac");
    ADD_CHECKPOINT(20000, "b5faccd48dfd909fee37f0cae5c89a8aef7dbd36463d75d0f7a8ed76a2cd3565");
    ADD_CHECKPOINT(25000, "940ef9acf8beede36af8c6cfdff4f37cce2ab8a6a32650d83dfd2b293e2c82eb");
    ADD_CHECKPOINT(30000, "6e5ac72bf1ec40c3b1b36f77b15638924dcf122cbcee2da868dff7ff7f96462c");
    ADD_CHECKPOINT(35000, "55942cac0b2ad39d1374389520c40832e57a3c2d3f794c866f833331bb0bfdb7");	
    ADD_CHECKPOINT(40000, "88669567b92f935f1184edcda288db16428bdaf0219a2753b2371913e5726dda");
    ADD_CHECKPOINT(45000, "0a1a9ca698336db7d952530fce36b52be0e5c00245b7ffb3ff2daf99337427db");
    ADD_CHECKPOINT(50000, "cc7bbc0f358a68f8bfd6e9aa6edde443e91e9bd72c679dda96285efc54b42686");
    ADD_CHECKPOINT(55000, "75c4b535ec26e0fc8dc6c6aa0d6c3da323b8ef4e7429361faed8fae2c0ba4cef");
    ADD_CHECKPOINT(60000, "53c402aa63485278d3694bb325aacc765a19e2d437cc247f3ff6cae39d8f15ad");
    ADD_CHECKPOINT(65000, "0a4a377a2b1e6794f466cf2821ad09822727dc422de2a12c8d67ae4e485a9d6c");
    ADD_CHECKPOINT(70000, "40c1b22c2c7534d51c07d68d89633e28905e9a2994f57bc8d8a6dda7e30a3add");
    ADD_CHECKPOINT(75000, "03349017c88d74cc464cdb009176796fe4f76e07d08729305ae9ac333d952849");
    ADD_CHECKPOINT(80000, "063caba36ea2fc92ef58368841416178cbc431a1a1a9722cc013eee521231efc");
    ADD_CHECKPOINT(84000, "98f625f06f698784f04720699e046de59576a59ba1dab52fb5d888e7494cb7dd");
    ADD_CHECKPOINT(84006, "8cfba655b62b0bcb0e3037c8c936bd1f66a614357ff4a5db6de2cd1da4dc7739");
    ADD_CHECKPOINT(84012, "a7dde898f522e331138a47841d865a3041e237088ce24631b7603a892e3e7d1a");
    ADD_CHECKPOINT(84024, "4d9a97ab0815aab1ed11ec73910b95399d93837d6525028bccc7f634e9305bf7");
    ADD_CHECKPOINT(84030, "d3c24286599cce09b6e9e48d7c3d7a05b421cdbe59625df60389cf6f5a99e8b3");
    ADD_CHECKPOINT(85000, "676d4981fe4add1e31ed84351e6b15da5ff20bb0b36509b62fb8d72ec3f43af1");
    ADD_CHECKPOINT(90000, "00fab750f72da9c609ec2d1d187ff4eeb21e53132afa7a68affd81806c2cbd29");
	  
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath);
    for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(bool testnet)
  {
    std::vector<std::string> records;

    // All four MyntPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = { "checkpoints.myntpulse.com"
						     , "checkpoints.myntpulse.org"
						     , "checkpoints.myntpulse.net"
						     , "checkpoints.myntpulse.info"
    };

    static const std::vector<std::string> testnet_dns_urls = { "testpoints.myntpulse.com"
							     , "testpoints.myntpulse.org"
							     , "testpoints.myntpulse.net"
							     , "testpoints.myntpulse.info"
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, testnet ? testnet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::parse_tpod_from_hex_string(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string json_hashfile_fullpath, bool testnet, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(testnet);
    }

    return result;
  }
}
