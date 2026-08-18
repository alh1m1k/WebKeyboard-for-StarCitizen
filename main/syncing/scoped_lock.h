#pragma once

#include <mutex>
#include <tuple>

namespace syncing {

	template <typename... MUX_TS>
	class scoped_lock {

		std::tuple<MUX_TS &...> mux;

		bool owns;

		void unlock() { std::apply([](auto&... m) { (m.unlock(), ...); }, mux); }

		public:

			explicit scoped_lock(MUX_TS&... mux) : mux(std::tie(mux...)) {
				std::lock(mux...);
				owns = true;
			}

			scoped_lock(std::adopt_lock_t, MUX_TS&... mux) : mux(std::tie(mux...)), owns(true) {	}

			scoped_lock(std::defer_lock_t, MUX_TS&... mux) : mux(std::tie(mux...)), owns(false) {	}

			scoped_lock(std::try_to_lock_t, MUX_TS&... mux)
				: mux(std::tie(mux...)), owns(std::try_lock(mux...) == -1) { }

			~scoped_lock() {
				if (owns) { unlock(); }
				owns = false;
			}

			scoped_lock(const scoped_lock&) = delete;
			scoped_lock& operator=(const scoped_lock&) = delete;

			void swap(scoped_lock& other) noexcept {
				std::swap(mux, other.mux);
				std::swap(owns, other.owns);
			}

			scoped_lock(scoped_lock&& move) noexcept : mux(move.mux), owns(move.owns)
			{
				move.owns = false;
				//move.mux = 0;
			}

			scoped_lock& operator=(scoped_lock&& move) noexcept
			{
				if (owns) { unlock(); }
				swap(move);

				//move.mux = 0;
				move.owns = false;

				return *this;
			}

			[[nodiscard]] bool owns_lock() const noexcept { return owns; }
			explicit operator bool() const noexcept { return owns_lock(); }
			auto* mutex() const noexcept { return &mux; }
	};

	template <typename MUX_T>
	class scoped_lock<MUX_T> {

		MUX_T& mux;

		bool owns;

		void unlock() { mux.unlock(); }

		public:

			explicit scoped_lock(MUX_T& mux) : mux(mux) {
				mux.lock();
				owns = true;
			}

			scoped_lock(std::adopt_lock_t, MUX_T& mux): mux(mux), owns(true) {	}

			scoped_lock(std::defer_lock_t, MUX_T& mux): mux(mux), owns(false) { }

			scoped_lock(std::try_to_lock_t, MUX_T& mux): mux(mux), owns(mux.try_lock()) { }

			~scoped_lock() {
				if (owns) { unlock(); }
				owns = false;
			}

			scoped_lock(const scoped_lock&) = delete;
			scoped_lock& operator=(const scoped_lock&) = delete;

			void swap(scoped_lock& other) noexcept {
				std::swap(mux, other.mux);
				std::swap(owns, other.owns);
			}

			scoped_lock(scoped_lock&& move) noexcept : mux(move.mux), owns(move.owns)
			{
				move.owns = false;
				//move.mux = 0;
			}

			scoped_lock& operator=(scoped_lock&& move) noexcept
			{
				if (owns) { unlock(); }
				swap(move);

				//move.mux = 0;
				move.owns = false;

				return *this;
			}

			[[nodiscard]] bool owns_lock() const noexcept { return owns; }
			explicit operator bool() const noexcept { return owns_lock(); }
			auto* mutex() const noexcept { return &mux; }
	};

}