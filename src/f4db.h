// Borrowed from Ryan's CommonLibF4
/* 
MIT License

Copyright (c) 2019 ryan-rsm-mckenzie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once


#include <map>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <execution>
#include <xutility>
#include <vector>
#include <format>
#include <span>
#include <mmio/mmio.hpp>
#include <stdexcept>

class IDDatabase
{
private:
	struct mapping_t
	{
		std::uint64_t id;
		std::uint64_t offset;
	};

public:
	IDDatabase(const IDDatabase&) = delete;
	IDDatabase(IDDatabase&&) = delete;

	IDDatabase& operator=(const IDDatabase&) = delete;
	IDDatabase& operator=(IDDatabase&&) = delete;

	class Offset2ID
	{
	public:
		using value_type = mapping_t;
		using container_type = std::vector<value_type>;
		using size_type = typename container_type::size_type;
		using const_iterator = typename container_type::const_iterator;
		using const_reverse_iterator = typename container_type::const_reverse_iterator;

		template <class ExecutionPolicy>
		explicit Offset2ID(ExecutionPolicy&& a_policy, std::span<const mapping_t> id2offset)  // NOLINT(bugprone-forwarding-reference-overload)
			requires(std::is_execution_policy_v<std::decay_t<ExecutionPolicy>>)
		{
			_offset2id.reserve(id2offset.size());
			_offset2id.insert(_offset2id.begin(), id2offset.begin(), id2offset.end());
			std::sort(
				a_policy,
				_offset2id.begin(),
				_offset2id.end(),
				[](auto&& a_lhs, auto&& a_rhs) {
				return a_lhs.id < a_rhs.id;
			});
		}

		Offset2ID(std::span<const mapping_t> id2offset) :
			Offset2ID(std::execution::sequenced_policy{}, id2offset)
		{}
		Offset2ID(){}

		[[nodiscard]] std::uint64_t operator()(std::size_t a_offset) const
		{
			if (_offset2id.empty()) {
				std::cerr << "data is empty";
			}

			const mapping_t elem{ 0, a_offset };
			const auto it = std::lower_bound(
				_offset2id.begin(),
				_offset2id.end(),
				elem,
				[](auto&& a_lhs, auto&& a_rhs) {
				return a_lhs.offset < a_rhs.offset;
			});
			if (it == _offset2id.end()) {
				std::cerr << "offset not found";
			}

			return it->id;
		}

		[[nodiscard]] const_iterator begin() const noexcept { return _offset2id.begin(); }
		[[nodiscard]] const_iterator cbegin() const noexcept { return _offset2id.cbegin(); }

		[[nodiscard]] const_iterator end() const noexcept { return _offset2id.end(); }
		[[nodiscard]] const_iterator cend() const noexcept { return _offset2id.cend(); }

		[[nodiscard]] const_reverse_iterator rbegin() const noexcept { return _offset2id.rbegin(); }
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept { return _offset2id.crbegin(); }

		[[nodiscard]] const_reverse_iterator rend() const noexcept { return _offset2id.rend(); }
		[[nodiscard]] const_reverse_iterator crend() const noexcept { return _offset2id.crend(); }

		[[nodiscard]] size_type size() const noexcept { return _offset2id.size(); }

	private:
		container_type _offset2id;
	};
	
	[[nodiscard]] std::size_t id2offset(std::uint64_t a_id) const
	{
		if (_id2offset.empty()) {
			std::cerr << "data is empty";
		}

		const mapping_t elem{ a_id, 0 };
		const auto it = std::lower_bound(
			_id2offset.begin(),
			_id2offset.end(),
			elem,
			[](auto&& a_lhs, auto&& a_rhs) {
			return a_lhs.id < a_rhs.id;
		});
		if (it == _id2offset.end()) {
			std::cerr << "offset not found";
		}

		return static_cast<std::size_t>(it->offset);
	}

protected:
	friend class Offset2ID;
	mmio::mapped_file_source _mmap;
	std::span<const mapping_t> _id2offset;
	Offset2ID offset2id;
	[[nodiscard]] std::span<const mapping_t> get_id2offset() const noexcept { return _id2offset; }

public:
	IDDatabase(const std::string& path) { load(path); }
	~IDDatabase() = default;

    void load(const std::string & path)
	{
		if (!_mmap.open(path)) {
			throw std::runtime_error(std::format("ERROR: failed to open: {}", path));
		}
		_id2offset = std::span{
			reinterpret_cast<const mapping_t*>(_mmap.data() + sizeof(std::uint64_t)),
			*reinterpret_cast<const std::uint64_t*>(_mmap.data())
		};
		offset2id = Offset2ID(_id2offset);
	}

	bool Dump(const std::string& path, bool use_base)
	{
		std::ofstream f = std::ofstream(path.c_str());
		if (!f.good())
			throw std::runtime_error(std::format("ERROR: failed to open {} for writing", path));
		const uint64_t base = 0x7FF6F21A0000;
		for (auto itr = offset2id.begin(); itr != offset2id.end(); ++itr)
		{
			auto offset = itr->offset;
			if (use_base){
				offset += base;
			}
			f << std::format("{:<10}\t0x{:12X}\n", itr->id, offset);
		}

		return true;
	}

};
