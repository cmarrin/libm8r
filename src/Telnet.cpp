/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Telnet.h"

#include "Value.h"

using namespace m8r;

Telnet::Telnet()
{
    _stateMachine.addState(State::Ready, []() { }, 
    {
          { Input::Backspace, State::Backspace }
        , { Input::Printable, State::AddChar }
        , { Input::Interrupt, State::Interrupt }
        , { Input::LF, State::Ready }
        , { Input::CR, State::SendLine }
        , { Input::CSI, State::CSI }
        , { Input::IAC, State::IAC }
    });
    
    _stateMachine.addState(State::Backspace, []() { handleBackspace(); }, State::Ready);
    _stateMachine.addState(State::AddChar, []() { handleAddChar(); }, State::Ready);
    _stateMachine.addState(State::Interrupt, []() { handleInterrupt(); }, State::Ready);
    _stateMachine.addState(State::CR, []() { sendLine(); }, State::Ready);
    _stateMachine.addState(State::LF, []() { }, State::Ready);

    _stateMachine.addState(State::CSI, []() { }, 
         { Input::Other, State::Ready }
       , { Input::CSIOpenBracket, State::CSIParam }
   });

     _stateMachine.addState(State::IAC, []() { }, 
          { Input::IAC, State::AddFF }
        , { Input::Any, State::IACParam }
    });

}

void Telnet::handleBackspace()
{
    if (!_line.empty() && _position > 0) {
        _position--;
        _line.erase(_line.begin() + _position);

        if (_position == _line.size()) {
            // At the end of line. Do the simple thing
            toChannel = "\x08 \x08";
        } else {
            toChannel = makeInputLine();
        }
    }    
}

void Telnet::handleAddChar()
{
    if (_position == _line.size()) {
        // At end of line, do the simple thing
        _line.push_back(fromChannel);
        _position++;
        toChannel = fromChannel;
    } else {
        _line.insert(_line.begin() + _position, fromChannel);
        _position++;
        makeInputLine();
    }
}

void Telnet::sendLine()
{
    toClient = String::join(_line);
    _line.clear();
    _position = 0;
    toChannel = "\r\n";
}

Telnet::Action Telnet::receive(char fromChannel, String& toChannel, String& toClient)
{
    // TODO: With the input set to raw, do we need to handle any IAC commands from Telnet?
    if (_state == State::Ready) {
        switch(fromChannel) {
            case '\xff': _state = State::ReceivedIAC; break;
            case '\e': _state = State::EscapeCSI; break;
            case '\x03':
                // ctl-c
                toClient = fromChannel;
                break;
            case '\n':
                break; // Ignore newline
            case '\r':
                // We have a complete line
                toClient = String::join(_line);
                _line.clear();
                _position = 0;
                toChannel = "\r\n";
                break;
            case '\x7f': // backspace
                if (!_line.empty() && _position > 0) {
                    _position--;
                    _line.erase(_line.begin() + _position);

                    if (_position == _line.size()) {
                        // At the end of line. Do the simple thing
                        toChannel = "\x08 \x08";
                    } else {
                        toChannel = makeInputLine();
                    }
                }
                break;
            default:
                if (_position == _line.size()) {
                    // At end of line, do the simple thing
                    _line.push_back(fromChannel);
                    _position++;
                    toChannel = fromChannel;
                } else {
                    _line.insert(_line.begin() + _position, fromChannel);
                    _position++;
                    makeInputLine();
                }
                break;
        }
    } else if (_state == State::ReceivedIAC) {
        switch(static_cast<Command>(fromChannel)) {
            case Command::DO: _verb = Verb::DO; break;
            case Command::DONT: _verb = Verb::DONT; break;
            case Command::WILL: _verb = Verb::WILL; break;
            case Command::WONT: _verb = Verb::WONT; break;
            case Command::IAC:
                _line.push_back(fromChannel);
                _state = State::Ready;
                return Action::None;
            default: _verb = Verb::None; break;
        }
        _state = State::ReceivedVerb;
        return Action::None;
    } else if (_state == State::ReceivedVerb) {
        // TODO: Handle Verbs
        _state = State::Ready;
        return Action::None;
    } else if (_state == State::EscapeCSI) {
        _state = (fromChannel == '[') ? State::EscapeParam : State::Ready;
    } else if (_state == State::EscapeParam) {
        if (fromChannel >= 0x20 && fromChannel <= 0x2f) {
            _escapeParam = fromChannel;
        } else {
            switch(fromChannel) {
                case 'D':
                    if (_position > 0) {
                        _position--;
                        toChannel = "\e[D";
                    }
                    break;
            }
            _state = State::Ready;
        }
    } else {
        _state = State::Ready;
    }
    
    return Action::None;
}

String Telnet::makeInputLine()
{
    String s = "\e[1000D\e[0K";
    s += String::join(_line);
    s += "\e[1000D";
    if (_position) {
        s += "\e[";
        
        // TODO: Need to move all this string processing stuff to String class
        s += Value::toString(_position);
        s += "C";
    }
    return s;
}
