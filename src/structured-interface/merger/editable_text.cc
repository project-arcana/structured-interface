#include "editable_text.hh"

void si::merger::editable_text::on_text_input(cc::string_view s)
{
    if (has_selection()) // replace selection
    {
        CC_ASSERT(_selection_start < _text.size());
        CC_ASSERT(_selection_start + _selection_count <= _text.size());

        _text = _text.substring(0, _selection_start) + //
                s +                                    //
                _text.subview(_selection_start, _text.size() - _selection_start - _selection_count);

        _cursor = _selection_start + s.size();
        deselect();
    }
    else // insert at cursor
    {
        CC_ASSERT(_cursor <= _text.size());

        _text = _text.substring(0, _cursor) + //
                s +                           //
                _text.subview(_cursor, _text.size() - _cursor);

        _cursor += s.size();
    }
}

void si::merger::editable_text::remove_prev_char()
{
    if (has_selection())
    {
        _text = _text.substring(0, _selection_start) + _text.subview(_selection_start + _selection_count);
        _selection_start = 0;
        _selection_count = 0;
        return;
    }

    if (_cursor == 0)
        return;

    _text = _text.substring(0, _cursor - 1) + _text.subview(_cursor);
    _cursor--;
}

void si::merger::editable_text::remove_next_char()
{
    if (has_selection())
    {
        _text = _text.substring(0, _selection_start) + _text.subview(_selection_start + _selection_count);
        _selection_start = 0;
        _selection_count = 0;
        return;
    }

    if (_cursor == _text.size())
        return;

    _text = _text.substring(0, _cursor) + _text.subview(_cursor + 1);
}

void si::merger::editable_text::select_all()
{
    _cursor = 0;
    _selection_start = 0;
    _selection_count = _text.size();
}

void si::merger::editable_text::deselect()
{
    _selection_start = 0;
    _selection_count = 0;
}

void si::merger::editable_text::move_cursor_left()
{
    deselect();
    if (_cursor > 0)
        --_cursor;
}

void si::merger::editable_text::move_cursor_right()
{
    deselect();
    if (_cursor < _text.size())
        ++_cursor;
}

void si::merger::editable_text::reset(cc::string new_text)
{
    _text = cc::move(new_text);
    _cursor = 0;
    _selection_start = 0;
    _selection_count = 0;
}
