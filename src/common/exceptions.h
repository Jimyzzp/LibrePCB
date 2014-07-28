/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Class Exception
 ****************************************************************************************/

/**
 * @brief The Exception class
 *
 * This is an exception base class. It inherits from the QException class, which implements
 * an exception class which can be transferred accross threads. QException inherits from
 * std::exception.
 *
 * @note Exceptions must always be thrown by value and caught by reference!
 *
 * For more information about the QException class read the Qt documentation.
 *
 * The Exception class adds member variables for a message, and the filename and line number
 * where the exception was thrown. They all must be passed to the constructor.
 *
 * @note Every exception will automatically print a debug message (see #Debug) of type
 *       Debug::DebugLevel::Exception.
 *
 * Example how to use exceptions:
 *
 * @code
 * void foo(int i)
 * {
 *     if (i < 0)
 *         throw Exception("i is negative!", __FILE__, __LINE__);
 * }
 *
 * void bar()
 * {
 *     try
 *     {
 *         foo(-5);
 *     }
 *     catch (Exception& e)
 *     {
 *         qDebug() << e.getMsg() << " (thrown in file " << e.getFile()
 *                  << " at line " << e.getLine() << ")";
 *         // or:
 *         qDebug() << e.getDebugString();
 *     }
 * }
 * @endcode
 *
 * @warning Please read the "Exception Safety" notes from the Qt Project documentation
 * before writing source code which throws exceptions! There are some important things
 * to know! http://qt-project.org/doc/qt-5/exceptionsafety.html
 */
class Exception : public QException
{
    public:

        // Constructor

        /**
         * @brief The constructor which is used to throw an exception
         *
         * @param msg   An error message (always in english!)
         * @param file  The source file where the exception was thrown (use __FILE__)
         * @param line  The line number where the exception was thrown (use __LINE__)
         */
        Exception(const QString& msg, const char* file, int line);


        // Getters

        /**
         * @brief Get the error message
         * @return The error message
         */
        const QString&  getMsg()    const {return mMsg;}

        /**
         * @brief Get the source file where the exception was thrown
         * @return The filename
         */
        const QString&  getFile()   const {return mFile;}

        /**
         * @brief Get the line number where the exception was thrown
         * @return The line number
         */
        int             getLine()   const {return mLine;}

        /**
         * @brief Get a debug string with all important informations of the exception
         *
         * @return A QString like "[LogicError] foobar not found!
         *                         (thrown in main.cpp at line 42)"
         */
        QString getDebugString() const;

        /**
         * @brief reimplemented from std::exception::what()
         *
         * @warning This method is only for compatibility reasons with the base class
         * std::exception. Normally, you should not use this method. Use getMsg() instead!
         *
         * @return the message as a C-string (const char*) in the local encoding
         */
        const char* what() const noexcept override;


        // Inherited from QException (see QException documentation for more details)
        virtual void raise() const {throw *this;}
        virtual Exception* clone() const {return new Exception(*this);}

    protected:

        // Attributes
        QString mMsg;   ///< the message of the exception (always in english!)
        QString mFile;  ///< the filename of the source file where the exception was thrown
        int mLine;      ///< the line number where the exception was thrown

    private:

        /// @brief make the default constructor inaccessible
        Exception();

};

/*****************************************************************************************
 *  Class LogicError
 ****************************************************************************************/

/**
 * @brief The LogicError class
 *
 * This exception class is used for exceptions related to the internal logic of the program.
 *
 * @see Exception
 */
class LogicError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        LogicError(const QString& msg, const char* file, int line);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual LogicError* clone() const {return new LogicError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        LogicError();
};

/*****************************************************************************************
 *  Class RuntimeError
 ****************************************************************************************/

/**
 * @brief The RuntimeError class
 *
 * This exception class is used for exceptions detected during runtime.
 *
 * @see Exception
 */
class RuntimeError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        RuntimeError(const QString& msg, const char* file, int line);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual RuntimeError* clone() const {return new RuntimeError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        RuntimeError();
};

/*****************************************************************************************
 *  Class RangeError
 ****************************************************************************************/

/**
 * @brief The RangeError class
 *
 * This exception class is used for range under-/overflows, for example in units.cpp.
 *
 * @see Exception
 */
class RangeError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        RangeError(const QString& msg, const char* file, int line);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual RangeError* clone() const {return new RangeError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        RangeError();
};

#endif // EXCEPTIONS_H
