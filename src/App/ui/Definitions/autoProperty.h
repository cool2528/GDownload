
#pragma once
#include <QObject>
template <typename T>
struct Compare;

template <typename T>
struct Compare {
	static bool isEqual(const T& t1, const T& t2) { return t1 == t2; }
};
template <>
struct Compare<float> {
	static bool isEqual(float f1, float f2) { return qFuzzyCompare(f1, f2); }
};
template <>
struct Compare<double> {
	static bool isEqual(double d1, double d2) { return qFuzzyCompare(d1, d2); }
};

#define PROP_DECL(TYPE, NAME) TYPE NAME##_;
#define PROP_DECL_V(TYPE, NAME, DEFAULT) TYPE NAME##_{DEFAULT};

#define PROP_GET(TYPE, NAME) \
	TYPE Get##NAME() const { \
		return NAME##_;      \
	}

#define PROP_SET(TYPE, NAME)                           \
	void Set##NAME(const TYPE& value) {                \
		if (!Compare<TYPE>::isEqual(NAME##_, value)) { \
			NAME##_ = value;                           \
			emit NAME##Changed(value);                 \
		}                                              \
	}

#define PROP_SIGNAL(TYPE, NAME) \
   Q_SIGNALS:                   \
	void NAME##Changed(TYPE value);

#define QML_AUTO_PROPERTY(TYPE, NAME)                          \
   public:                                                     \
	PROP_GET(TYPE, NAME)                                       \
   Q_SLOT PROP_SET(TYPE, NAME) PROP_SIGNAL(TYPE, NAME) private \
	   : Q_PROPERTY(TYPE NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) PROP_DECL(TYPE, NAME)

#define QML_AUTO_PROPERTY_V(TYPE, NAME, DEFAULT)               \
   public:                                                     \
	PROP_GET(TYPE, NAME)                                       \
   Q_SLOT PROP_SET(TYPE, NAME) PROP_SIGNAL(TYPE, NAME) private \
	   : Q_PROPERTY(TYPE NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) PROP_DECL_V(TYPE, NAME, DEFAULT)

#define QML_READ_PROPERTY(TYPE, NAME)                                         \
   public:                                                                    \
	PROP_GET(TYPE, NAME)                                                      \
	PROP_SET(TYPE, NAME)                                                      \
	PROP_SIGNAL(TYPE, NAME)                                                   \
   private:                                                                   \
	Q_PROPERTY(TYPE NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) \
	PROP_DECL(TYPE, NAME)

#define QML_READ_PROPERTY_V(TYPE, NAME, DEFAULT)                              \
   public:                                                                    \
	PROP_GET(TYPE, NAME)                                                      \
	PROP_SET(TYPE, NAME)                                                      \
	PROP_SIGNAL(TYPE, NAME)                                                   \
   private:                                                                   \
	Q_PROPERTY(TYPE NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) \
	PROP_DECL_V(TYPE, NAME, DEFAULT)

#define QML_READ_ONLY_PROPERTY(TYPE, NAME) \
   public:                                 \
	PROP_GET(TYPE, NAME)                   \
   private:                                \
	PROP_DECL(TYPE, NAME)

#define QML_READ_ONLY_PROPERTY_V(TYPE, NAME, DEFAULT) \
   public:                                            \
	PROP_GET(TYPE, NAME)                              \
   private:                                           \
	PROP_DECL_V(TYPE, NAME, DEFAULT)

#define QML_PROPERTY(TYPE, NAME, SET, GET) \
   public:                                 \
	Q_SLOT void SET();                     \
	TYPE GET();                            \
   PROP_SIGNAL(TYPE, NAME) private : PROP_DECL(TYPE, NAME)\
