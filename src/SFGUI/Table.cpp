#include <SFGUI/Table.hpp>

#include <set>

namespace sfg {

Table::Table() {
	OnSizeAllocate.Connect( &Table::HandleSizeAllocate, this );
}

Table::Ptr Table::Create() {
	return Ptr( new Table );
}

sf::Vector2f Table::GetRequisitionImpl() const {
	sf::Vector2f size( 0.f, 0.f );

	// Update requisitions.
	UpdateRequisitions();

	// Count allocations of columns.
	TableOptionsArray::const_iterator column_iter( m_columns.begin() );
	TableOptionsArray::const_iterator column_iter_end( m_columns.end() );

	for( ; column_iter != column_iter_end; ++column_iter ) {
		size.x += column_iter->requisition;
	}

	// Count allocations of rows.
	TableOptionsArray::const_iterator row_iter( m_rows.begin() );
	TableOptionsArray::const_iterator row_iter_end( m_rows.end() );

	for( ; row_iter != row_iter_end; ++row_iter ) {
		size.y += row_iter->requisition;
	}

	return size;
}

void Table::Attach( Widget::Ptr widget, const sf::Rect<sf::Uint32>& rect, int x_options, int y_options, const sf::Vector2f& padding ) {
	assert( rect.Width > 0 );
	assert( rect.Height > 0 );

	// Store widget in a table cell object.
	priv::TableCell cell( widget, rect, x_options, y_options, padding );
	m_cells.push_back( cell );

	// Check if we need to enlarge rows/columns.
	if( rect.Left + rect.Width >= m_columns.size() ) {
		m_columns.resize( rect.Left + rect.Width );
	}

	if( rect.Top + rect.Height >= m_rows.size() ) {
		m_rows.resize( rect.Top + rect.Height );
	}

	// Add widget to container.
	Add( widget );

	// Request new size.
	RequestSize();
}


void Table::HandleSizeAllocate( Widget::Ptr widget, const sf::FloatRect& /*old_allocation*/ ) {
	AllocateChildrenSizes();
}

void Table::UpdateRequisitions() const {
	// Reset requisitions and expand flags, at first.
	for( std::size_t column_index = 0; column_index < m_columns.size(); ++column_index ) {
		m_columns[column_index].requisition = 0.f;
		m_columns[column_index].allocation = 0.f;
		m_columns[column_index].expand = false;
	}

	for( std::size_t row_index = 0; row_index < m_rows.size(); ++row_index ) {
		m_rows[row_index].requisition = 0.f;
		m_rows[row_index].allocation = 0.f;
		m_rows[row_index].expand = false;
	}

	// Iterate over children to get the maximum requisition for each column and
	// row and set expand flags.
	TableCellList::const_iterator cell_iter( m_cells.begin() );
	TableCellList::const_iterator cell_iter_end( m_cells.end() );

	for( ; cell_iter != cell_iter_end; ++cell_iter ) {
		m_columns[cell_iter->rect.Left].requisition = std::max(
			m_columns[cell_iter->rect.Left].requisition,
			cell_iter->child->GetRequisition().x + m_columns[cell_iter->rect.Left].spacing + 2.f * cell_iter->padding.x
		);

		if( !m_columns[cell_iter->rect.Left].expand ) {
			m_columns[cell_iter->rect.Left].expand = (cell_iter->x_options & EXPAND);
		}

		m_rows[cell_iter->rect.Top].requisition = std::max(
			m_rows[cell_iter->rect.Top].requisition,
			cell_iter->child->GetRequisition().y + m_rows[cell_iter->rect.Top].spacing + 2.f * cell_iter->padding.y
		);

		if( !m_rows[cell_iter->rect.Top].expand ) {
			m_rows[cell_iter->rect.Top].expand = (cell_iter->y_options & EXPAND);
		}
	}
}

void Table::AllocateChildrenSizes() {
	// Process columns.
	float width( 0.f );
	std::size_t num_expand( 0 );

	// Calculate "normal" width of columns and count expanded columns.
	for( std::size_t column_index = 0; column_index < m_columns.size(); ++column_index ) {
		// Every allocaction will be at least as wide as the requisition.
		m_columns[column_index].allocation = m_columns[column_index].requisition;

		// Calc position.
		if( column_index == 0 ) {
			m_columns[column_index].position = 0.f;
		}
		else {
			m_columns[column_index].position = m_columns[column_index - 1].position + m_columns[column_index - 1].allocation;
		}

		// Count expanded columns.
		if( m_columns[column_index].expand ) {
			++num_expand;
		}

		width += m_columns[column_index].requisition;
	}

	// If there're expanded columns, we need to set the proper allocation for them.
	if( num_expand > 0 ) {
		float extra( (GetAllocation().Width - width) / static_cast<float>( num_expand ) );

		for( std::size_t column_index = 0; column_index < m_columns.size(); ++column_index ) {
			if( !m_columns[column_index].expand ) {
				continue;
			}

			m_columns[column_index].allocation += extra;

			// Position of next column must be increased.
			if( column_index + 1 < m_columns.size() ) {
				m_columns[column_index + 1].position += extra;
			}
		}
	}

	// Process rows.
	float height( 0.f );
	num_expand = 0;

	// Calculate "normal" height of columns and count expanded rows.
	for( std::size_t row_index = 0; row_index < m_rows.size(); ++row_index ) {
		// Every allocaction will be at least as wide as the requisition.
		m_rows[row_index].allocation = m_rows[row_index].requisition;

		// Calc position.
		if( row_index == 0 ) {
			m_rows[row_index].position = 0.f;
		}
		else {
			m_rows[row_index].position = m_rows[row_index - 1].position + m_rows[row_index - 1].allocation;
		}

		// Count expanded rows.
		if( m_rows[row_index].expand ) {
			++num_expand;
		}

		height += m_rows[row_index].requisition;
	}

	// If there're expanded rows, we need to set the proper allocation for them.
	if( num_expand > 0 ) {
		float extra( (GetAllocation().Height - height) / static_cast<float>( num_expand ) );

		for( std::size_t row_index = 0; row_index < m_rows.size(); ++row_index ) {
			if( !m_rows[row_index].expand ) {
				continue;
			}

			m_rows[row_index].allocation += extra;

			// Position of next row must be increased.
			if( row_index + 1 < m_rows.size() ) {
				m_rows[row_index + 1].position += extra;
			}
		}
	}

	// Allocate children sizes.
	TableCellList::iterator cell_iter( m_cells.begin() );
	TableCellList::iterator cell_iter_end( m_cells.end() );

	for( ; cell_iter != cell_iter_end; ++cell_iter ) {
		// Shortcuts.
		const priv::TableOptions& column( m_columns[cell_iter->rect.Left] );
		const priv::TableOptions& row( m_rows[cell_iter->rect.Top] );

		// Check for FILL flag.
		cell_iter->child->AllocateSize(
			sf::FloatRect(
				column.position + cell_iter->padding.x,
				row.position + cell_iter->padding.y,
				(cell_iter->x_options & FILL) ? column.allocation - column.spacing - 2.f * cell_iter->padding.x : cell_iter->child->GetRequisition().x,
				(cell_iter->y_options & FILL) ? row.allocation - row.spacing - 2.f * cell_iter->padding.y : cell_iter->child->GetRequisition().y
			)
		);
	}
}

void Table::SetColumnSpacings( float spacing ) {
	for( std::size_t column_index = 0; column_index < m_columns.size(); ++column_index ) {
		m_columns[column_index].spacing = spacing;
	}

	RequestSize();
}

void Table::SetRowSpacings( float spacing ) {
	for( std::size_t row_index = 0; row_index < m_rows.size(); ++row_index ) {
		m_rows[row_index].spacing = spacing;
	}

	RequestSize();
}

void Table::SetColumnSpacing( std::size_t index, float spacing ) {
	if( index >= m_columns.size() ) {
		return;
	}

	m_columns[index].spacing = spacing;
	RequestSize();
}

void Table::SetRowSpacing( std::size_t index, float spacing ) {
	if( index >= m_rows.size() ) {
		return;
	}

	m_rows[index].spacing = spacing;
	RequestSize();
}

}