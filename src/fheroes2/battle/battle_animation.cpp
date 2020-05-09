/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "battle_animation.h"
#include "monster.h"
#include "settings.h"
#include <algorithm>

AnimationSequence::AnimationSequence( const std::vector<int> & seq )
    : _seq( seq )
    , _currentFrame( 0 )
{}

AnimationSequence & AnimationSequence::operator=( const std::vector<int> & rhs )
{
    _seq = rhs;
    _currentFrame = 0;
    return *this;
}

int AnimationSequence::playAnimation( bool loop )
{
    if ( !isValid() )
        return 0;

    if ( isLastFrame() ) {
        if ( loop )
            restartAnimation();
    }
    else {
        ++_currentFrame;
    }
    return _seq[_currentFrame];
}

int AnimationSequence::restartAnimation()
{
    _currentFrame = 0;
    return getFrame();
}

int AnimationSequence::getFrame() const
{
    return isValid() ? _seq[_currentFrame] : 0;
}

int AnimationSequence::animationLength() const
{
    return static_cast<int>( _seq.size() );
}

int AnimationSequence::firstFrame() const
{
    return isValid() ? _seq.front() : 0;
}

void AnimationSequence::setToLastFrame()
{
    if ( isValid() )
        _currentFrame = _seq.size() - 1;
}

double AnimationSequence::movementProgress() const
{
    if ( _seq.size() > 1 )
        return static_cast<double>( _currentFrame ) / ( static_cast<double>( animationLength() ) - 1 );

    return 0;
}

bool AnimationSequence::isFirstFrame() const
{
    return _currentFrame == 0;
}

bool AnimationSequence::isLastFrame() const
{
    return _currentFrame == _seq.size() - 1;
}

bool AnimationSequence::isValid() const
{
    return _seq.size() > 0;
}

AnimationReference::AnimationReference()
    : _monsterID( Monster::UNKNOWN )
{
}

AnimationReference::AnimationReference( int monsterID )
    : _monsterID( monsterID )
{
    if ( monsterID < Monster::PEASANT && monsterID > Monster::WATER_ELEMENT )
        return;

    _monsterInfo = Bin_Info::GetMonsterInfo( monsterID );

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( _static, Bin_Info::MonsterAnimInfo::STATIC ) ) {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

    // Taking damage
    appendFrames( _wince, Bin_Info::MonsterAnimInfo::WINCE_UP );
    appendFrames( _wince, Bin_Info::MonsterAnimInfo::WINCE_END ); // TODO: play it back together for now
    appendFrames( _death, Bin_Info::MonsterAnimInfo::DEATH );

    // Idle animations
    for ( uint32_t idx = Bin_Info::MonsterAnimInfo::IDLE1; idx < _monsterInfo.idleAnimationCount + Bin_Info::MonsterAnimInfo::IDLE1; ++idx ) {
        std::vector<int> idleAnim;

        if ( appendFrames( idleAnim, idx ) ) {
            _idle.push_back( idleAnim );
        }
    }

    // Movement sequences
    // Every unit has MOVE_MAIN anim, use it as a base
    appendFrames( _loopMove, Bin_Info::MonsterAnimInfo::MOVE_MAIN );

    if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::MOVE_ONE ) ) {
        appendFrames( _quickMove, Bin_Info::MonsterAnimInfo::MOVE_ONE );
    }
    else { // TODO: this must be LICH or POWER_LICH. Check it!
        _quickMove = _loopMove;
    }

    appendFrames( _moveModes.start, Bin_Info::MonsterAnimInfo::MOVE_START );
    appendFrames( _moveModes.end, Bin_Info::MonsterAnimInfo::MOVE_STOP );

    // Attack sequences
    appendFrames( _melee[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::ATTACK1 );
    appendFrames( _melee[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::ATTACK1_END );

    appendFrames( _melee[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::ATTACK2 );
    appendFrames( _melee[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::ATTACK2_END );

    appendFrames( _melee[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::ATTACK3 );
    appendFrames( _melee[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::SHOOT2 ) ) {
        appendFrames( _ranged[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::SHOOT1 );
        appendFrames( _ranged[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::SHOOT1_END );

        appendFrames( _ranged[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::SHOOT2 );
        appendFrames( _ranged[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::SHOOT2_END );

        appendFrames( _ranged[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::SHOOT3 );
        appendFrames( _ranged[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::SHOOT3_END );
    }
    else if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::DOUBLEHEX2 ) ) {
        // Only 6 units should have this (in the original game)
        appendFrames( _ranged[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX1 );
        appendFrames( _ranged[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX1_END );

        appendFrames( _ranged[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX2 );
        appendFrames( _ranged[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX2_END );

        appendFrames( _ranged[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX3 );
        appendFrames( _ranged[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX3_END );
    }

    _offsetX = _monsterInfo.frameXOffset;
}

AnimationReference::~AnimationReference() {}

bool AnimationReference::appendFrames( std::vector<int> & target, int animID )
{
    if ( _monsterInfo.hasAnim( animID ) ) {
        target.insert( target.end(), _monsterInfo.animationFrames.at( animID ).begin(), _monsterInfo.animationFrames.at( animID ).end() );
        return true;
    }
    return false;
}

const std::vector<int> & AnimationReference::getAnimationVector( int animState ) const
{
    switch ( animState ) {
    case Monster_Info::STATIC:
        return _static;
    case Monster_Info::IDLE:
        return _idle.front(); // TODO: use all idle animations
    case Monster_Info::MOVE_START:
        return _moveModes.start;
    case Monster_Info::MOVING:
        return _loopMove;
    case Monster_Info::MOVE_END:
        return _moveModes.end;
    case Monster_Info::MOVE_QUICK:
        return _quickMove;
    case Monster_Info::MELEE_TOP:
        return _melee[Monster_Info::TOP].start;
    case Monster_Info::MELEE_TOP_END:
        return _melee[Monster_Info::TOP].end;
    case Monster_Info::MELEE_FRONT:
        return _melee[Monster_Info::FRONT].start;
    case Monster_Info::MELEE_FRONT_END:
        return _melee[Monster_Info::FRONT].end;
    case Monster_Info::MELEE_BOT:
        return _melee[Monster_Info::BOTTOM].start;
    case Monster_Info::MELEE_BOT_END:
        return _melee[Monster_Info::BOTTOM].end;
    case Monster_Info::RANG_TOP:
        return _ranged[Monster_Info::TOP].start;
    case Monster_Info::RANG_TOP_END:
        return _ranged[Monster_Info::TOP].end;
    case Monster_Info::RANG_FRONT:
        return _ranged[Monster_Info::FRONT].start;
    case Monster_Info::RANG_FRONT_END:
        return _ranged[Monster_Info::FRONT].end;
    case Monster_Info::RANG_BOT:
        return _ranged[Monster_Info::BOTTOM].start;
    case Monster_Info::RANG_BOT_END:
        return _ranged[Monster_Info::BOTTOM].end;
    case Monster_Info::WNCE:
        return _wince;
    case Monster_Info::KILL:
        return _death;
    default:
        DEBUG( DBG_ENGINE, DBG_WARN, "Trying to display deprecated Animation " << animState );
        break;
    }
    return _static;
}

std::vector<int> AnimationReference::getAnimationOffset( int animState ) const
{
    std::vector<int> offset;
    switch ( animState ) {
    case Monster_Info::STATIC:
        offset.resize( _static.size(), 0 );
        break;
    case Monster_Info::IDLE:
        offset.resize( _idle.front().size(), 0 ); // TODO: use all idle animations
        break;
    case Monster_Info::MOVE_START:
        return _offsetX[Bin_Info::MonsterAnimInfo::MOVE_START];
    case Monster_Info::MOVING:
        return _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN];
        break;
    case Monster_Info::MOVE_END:
        return _offsetX[Bin_Info::MonsterAnimInfo::MOVE_STOP];
        break;
    case Monster_Info::MOVE_QUICK:
        offset.resize( _quickMove.size(), 0 );
        break;
    case Monster_Info::MELEE_TOP:
        offset.resize( _melee[Monster_Info::TOP].start.size(), 0 );
        break;
    case Monster_Info::MELEE_TOP_END:
        offset.resize( _melee[Monster_Info::TOP].end.size(), 0 );
        break;
    case Monster_Info::MELEE_FRONT:
        offset.resize( _melee[Monster_Info::FRONT].start.size(), 0 );
        break;
    case Monster_Info::MELEE_FRONT_END:
        offset.resize( _melee[Monster_Info::FRONT].end.size(), 0 );
        break;
    case Monster_Info::MELEE_BOT:
        offset.resize( _melee[Monster_Info::BOTTOM].start.size(), 0 );
        break;
    case Monster_Info::MELEE_BOT_END:
        offset.resize( _melee[Monster_Info::BOTTOM].end.size(), 0 );
        break;
    case Monster_Info::RANG_TOP:
        offset.resize( _ranged[Monster_Info::TOP].start.size(), 0 );
        break;
    case Monster_Info::RANG_TOP_END:
        offset.resize( _ranged[Monster_Info::TOP].end.size(), 0 );
        break;
    case Monster_Info::RANG_FRONT:
        offset.resize( _ranged[Monster_Info::FRONT].start.size(), 0 );
        break;
    case Monster_Info::RANG_FRONT_END:
        offset.resize( _ranged[Monster_Info::FRONT].end.size(), 0 );
        break;
    case Monster_Info::RANG_BOT:
        offset.resize( _ranged[Monster_Info::BOTTOM].start.size(), 0 );
        break;
    case Monster_Info::RANG_BOT_END:
        offset.resize( _ranged[Monster_Info::BOTTOM].end.size(), 0 );
        break;
    case Monster_Info::WNCE:
        offset.resize( _wince.size(), 0 );
        break;
    case Monster_Info::KILL:
        offset.resize( _death.size(), 0 );
        break;
    default:
        DEBUG( DBG_ENGINE, DBG_WARN, "Trying to use deprecated Animation " << animState );
        break;
    }
    return offset;
}

AnimationSequence AnimationReference::getAnimationSequence( int animState ) const
{
    return AnimationSequence( getAnimationVector( animState ) );
}

int AnimationReference::getStaticFrame() const
{
    return _static.back();
}

int AnimationReference::getDeathFrame() const
{
    return ( _death.empty() ) ? _static.back() : _death.back();
}

AnimationState::AnimationState( int monsterID )
    : AnimationReference( monsterID )
    , _currentSequence( _static )
    , _animState( Monster_Info::STATIC )
{}

AnimationState::AnimationState( const AnimationReference & ref, int state )
    : AnimationReference( ref )
    , _currentSequence( _static )
{
    switchAnimation( state );
}

AnimationState::~AnimationState() {}

bool AnimationState::switchAnimation( int animState, bool reverse )
{
    std::vector<int> seq = getAnimationVector( animState );
    if ( seq.size() > 0 ) {
        _animState = animState;
        if ( reverse )
            std::reverse( seq.begin(), seq.end() );
        _currentSequence = seq;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim " << animState << " length " << _currentSequence.animationLength() );
    }
    return false;
}

bool AnimationState::switchAnimation( const std::vector<int> & animationList, bool reverse )
{
    std::vector<int> combinedAnimation;

    for ( std::vector<int>::const_iterator it = animationList.begin(); it != animationList.end(); ++it ) {
        const std::vector<int> & seq = getAnimationVector( *it );
        if ( seq.size() > 0 ) {
            _animState = *it;
            combinedAnimation.insert( combinedAnimation.end(), seq.begin(), seq.end() );
        }
    }

    if ( combinedAnimation.size() > 0 ) {
        if ( reverse )
            std::reverse( combinedAnimation.begin(), combinedAnimation.end() );

        _currentSequence = combinedAnimation;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim list of length " << animationList.size() );
    }
    return false;
}

int AnimationState::getCurrentState() const
{
    return _animState;
}

int AnimationState::playAnimation( bool loop )
{
    return _currentSequence.playAnimation( loop );
}

int AnimationState::restartAnimation()
{
    return _currentSequence.restartAnimation();
}

int AnimationState::getFrame() const
{
    return _currentSequence.getFrame();
}

int AnimationState::animationLength() const
{
    return _currentSequence.animationLength();
}

int AnimationState::firstFrame() const
{
    return _currentSequence.firstFrame();
}

void AnimationState::setToLastFrame()
{
    return _currentSequence.setToLastFrame();
}

double AnimationState::movementProgress() const
{
    return _currentSequence.movementProgress();
}

bool AnimationState::isFirstFrame() const
{
    return _currentSequence.isFirstFrame();
}

bool AnimationState::isLastFrame() const
{
    return _currentSequence.isLastFrame();
}

bool AnimationState::isValid() const
{
    return _currentSequence.isValid();
}
