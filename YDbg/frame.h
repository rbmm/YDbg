#pragma once

template<typename T> struct RTL_FRAME : TEB_ACTIVE_FRAME, public T 
{
	static const TEB_ACTIVE_FRAME_CONTEXT* getContext()
	{
		static const TEB_ACTIVE_FRAME_CONTEXT s = { 0, __FUNCDNAME__ };
		return &s;
	}

	~RTL_FRAME()
	{
		RtlPopFrame(this);
	}

	template<typename... Types>
	RTL_FRAME(Types... args) : T(args...)
	{
		Context = getContext();
		Flags = 0;
		RtlPushFrame(this);
	}

	static T* get()
	{
		if (TEB_ACTIVE_FRAME* prf = RtlGetFrame())
		{
			const TEB_ACTIVE_FRAME_CONTEXT* ctx = getContext();
			do 
			{
				if (prf->Context == ctx) return static_cast<RTL_FRAME*>(prf);
			} while (prf = prf->Previous);
		}

		return 0;
	}
};