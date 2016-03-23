#include "listitems.h"

#include "buttonpanel.h"
#include "item.h"
#include "modelitems.h"
#include "viewitems.h"
#include "watch.h"

#include <QtCore/QEvent>
#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>

#define AFOUTPUT
#undef AFOUTPUT
#include "../include/macrooutput.h"

ListItems::ListItems( QWidget* parent, const std::string & type, int RequestMsgType):
	QWidget( parent),
	m_type( type),
	m_parentWindow( parent),
	m_requestmsgtype( RequestMsgType),
	m_subscribed( false),
	m_subscribeFirstTime( true)
{
AFINFO("ListItems::ListItems.\n");
	setAttribute ( Qt::WA_DeleteOnClose, true );

	m_hlayout = new QHBoxLayout( this);
	m_vlayout = new QVBoxLayout();
	m_panel_l = new QVBoxLayout();
	m_panel_r = new QVBoxLayout();
	m_hlayout->addLayout( m_panel_l);
	m_hlayout->addLayout( m_vlayout);
	m_hlayout->addLayout( m_panel_r);

	m_panel_l->setAlignment( Qt::AlignTop);
	m_panel_l->setContentsMargins( 5, 5, 5, 5);
	m_panel_l->setSpacing( 5);

//m_panel_l->addWidget(new QLabel("left"));
//m_panel_r->addWidget(new QLabel("right"));

	m_hlayout->setSpacing( 0);
	m_vlayout->setSpacing( 0);
#if QT_VERSION >= 0x040300
	m_hlayout->setContentsMargins( 0, 0, 0, 0);
	m_vlayout->setContentsMargins( 0, 0, 0, 0);
#endif
	m_infoline = new InfoLine( this);

//	if( m_parentWindow != (QWidget*)(Watch::getDialog())) setFocusPolicy(Qt::StrongFocus);
	setFocusPolicy(Qt::StrongFocus);
}

bool ListItems::init( bool createModelView)
{
	if( createModelView)
	{
		m_model = new ModelItems(this);
		m_view = new ViewItems( this);
		m_view->setModel( m_model);

		m_vlayout->addWidget( m_view);
		m_vlayout->addWidget( m_infoline);
	}

	connect( m_view, SIGNAL(doubleClicked( const QModelIndex &)), this, SLOT( doubleClicked_slot( const QModelIndex &)));

	connect( m_view->selectionModel(), SIGNAL(     currentChanged( const QModelIndex &, const QModelIndex &)),
	                             this, SLOT(   currentItemChanged( const QModelIndex &, const QModelIndex &)));

	return true;
}

ListItems::~ListItems()
{
AFINFO("ListItems::~ListItems.\n");

	Watch::unsubscribe( m_eventsOnOff);
}

int ListItems::count() const { return m_model->count();}

bool ListItems::mousePressed( QMouseEvent * event) { return false;}

void ListItems::showEvent( QShowEvent * event)
{
	v_shownFunc();
}

void ListItems::v_shownFunc()
{
	if( Watch::isConnected() && m_requestmsgtype ) Watch::sendMsg( new af::Msg( m_requestmsgtype, 0, true));
}

void ListItems::hideEvent( QHideEvent * event)
{
	v_unSubscribe();
}

void ListItems::v_subscribe()
{
	if( m_subscribed) return;
	if( m_subscribeFirstTime )
	{
		QList<int> eventsAll;
		eventsAll << m_eventsOnOff;
		eventsAll << m_eventsShowHide;
		Watch::subscribe( eventsAll);
		m_subscribeFirstTime = false;
	}
	else
	{
		Watch::subscribe( m_eventsShowHide);
	}
	m_subscribed = true;
}

void ListItems::v_unSubscribe()
{
	if( m_subscribed == false) return;
	Watch::unsubscribe( m_eventsShowHide);
	m_subscribed = false;
}

void ListItems::v_connectionLost()
{
	v_unSubscribe();
	Watch::unsubscribe( m_eventsOnOff);
	deleteAllItems();
}

void ListItems::v_connectionEstablished() { if( isVisible()) v_shownFunc(); }
void ListItems::deleteAllItems() { m_model->deleteAllItems();}
void ListItems::doubleClicked( Item * item) {}
void ListItems::revertModel()  { m_model->revert();}
void ListItems::itemsHeightChanged() { m_model->itemsHeightChanged();}
void ListItems::itemsHeightCalc() { m_model->itemsHeightCalc();}
void ListItems::repaintItems() { m_view->repaintViewport();}

void ListItems::deleteItems( const std::vector<int32_t> & i_ids)
{
	int row = 0;
	while( row < m_model->count())
	{
		bool deleted = false;
		for( int i = 0; i < i_ids.size(); i++)
		{
			if( i_ids[i] == m_model->item(row)->getId())
			{
				m_model->delItem( row);
				deleted = true;
				break;
			}
		}

		if( deleted )
			continue;

		row++;
	}
}
void ListItems::setAllowSelection( bool allow)
{
	if( allow ) m_view->setSelectionMode( QAbstractItemView::ExtendedSelection  );
	else        m_view->setSelectionMode( QAbstractItemView::NoSelection        );
}

Item* ListItems::getCurrentItem() const
{
	QModelIndex index( m_view->selectionModel()->currentIndex());
	if( false == index.isValid()) return NULL;
	if( false == qVariantCanConvert<Item*>(index.data())) return NULL;
	return qVariantValue<Item*>( index.data());
}

int ListItems::getSelectedItemsCount() const
{
	return m_view->selectionModel()->selectedIndexes().size();
}

const QList<Item*> ListItems::getSelectedItems() const
{
	QList<Item*> items;

	QModelIndexList indexes( m_view->selectionModel()->selectedIndexes());
	for( int i = 0; i < indexes.count(); i++)
		if( qVariantCanConvert<Item*>( indexes[i].data()))
			items << qVariantValue<Item*>( indexes[i].data());

	return items;
}

void ListItems::setSelectedItems( const QList<Item*> & items, bool resetSelection)
{
	if( resetSelection ) m_view->clearSelection();
	if( items.count() < 1 ) return;
	int modelcount = m_model->count();
	int lastselectedrow = -1;
	for( int i = 0; i < modelcount; i++)
	{
		if( items.contains( m_model->item(i)))
		{
			m_view->selectionModel()->select( m_model->index(i), QItemSelectionModel::Select);
			lastselectedrow = i;
		}
	}
	if( lastselectedrow != -1)
		m_view->selectionModel()->setCurrentIndex( m_model->index(lastselectedrow), QItemSelectionModel::Current);
}

void ListItems::action( af::MCGeneral& mcgeneral, int type)
{
	QModelIndexList indexes( m_view->selectionModel()->selectedIndexes());
	for( int i = 0; i < indexes.count(); i++)
		if( qVariantCanConvert<Item*>( indexes[i].data()))
			mcgeneral.addId( qVariantValue<Item*>( indexes[i].data())->getId());

	if( mcgeneral.getCount() == 0) return;
//printf("ListNodes::action:\n"); mcgeneral.stdOut( true);

	af::Msg * msg = new af::Msg( type, &mcgeneral);

	Watch::sendMsg( msg);
}

void ListItems::doubleClicked_slot( const QModelIndex & index )
{
	if( qVariantCanConvert<Item*>( index.data())) doubleClicked( qVariantValue<Item*>( index.data()));
}

void ListItems::currentItemChanged( const QModelIndex & current, const QModelIndex & previous )
{
	if( qVariantCanConvert<Item*>( current.data()))
		displayInfo( qVariantValue<Item*>( current.data())->getSelectString());
}

void ListItems::setParameter( const std::string & i_name, const std::string & i_value, bool i_quoted) const
{
	std::ostringstream str;

	af::jsonActionParamsStart( str, m_type, "", getSelectedIds());

	str << "\n\"" << i_name << "\":";
	if( i_quoted ) str << "\"";
	str << i_value;
	if( i_quoted ) str << "\"";

	af::jsonActionParamsFinish( str);

	Watch::sendMsg( af::jsonMsg( str));
}

void ListItems::operation( const std::string & i_operation)
{
	std::ostringstream str;
	std::vector<int> ids = getSelectedIds();
	if( ids.size() == 0 )
	{
		displayWarning("No items selected.");
		return;
	}
	af::jsonActionOperation( str, m_type, i_operation, "", ids);
	Watch::sendMsg( af::jsonMsg( str));

	displayInfo(QString("Operation: \"%1\".").arg( afqt::stoq( i_operation)));
}

void ListItems::get(
		const char * i_type,
		const std::vector<int32_t> & i_ids,
		const std::vector<std::string> & i_modes,
		const std::vector<int32_t> & i_blocks)
{
//	{"get":{"type":"jobs","ids":[6],"mode":["progress"],"block_ids":[0]}}
	std::ostringstream str;

	str << "{\"get\":{\"binary\":true,\"type\":\"" << i_type << "\",\"ids\":[";
	for( int i = 0; i < i_ids.size(); i++ ) { if(i) str << ','; str << i_ids[i]; }
	str << "]";

	if( i_modes.size())
	{
		str << ",\"mode\":[";
		for( int i = 0; i < i_modes.size(); i++ ) { if(i) str << ','; str << '"' << i_modes[i] << '"'; }
		str << "]";
	}

	if( i_blocks.size())
	{
		str << ",\"block_ids\":[";
		for( int i = 0; i < i_blocks.size(); i++ ) { if(i) str << ','; str << i_blocks[i]; }
		str << "]";
	}

	str << "}}";

	af::Msg * msg = af::jsonMsg( str);

	msg->setReceiving( true);

	Watch::sendMsg( msg);
}

const std::vector<int> ListItems::getSelectedIds() const
{
	std::vector<int> ids;
	QModelIndexList indexes( m_view->selectionModel()->selectedIndexes());
	for( int i = 0; i < indexes.count(); i++)
		if( qVariantCanConvert<Item*>( indexes[i].data()))
			ids.push_back( qVariantValue<Item*>( indexes[i].data())->getId());
	return ids;
}

ButtonPanel * ListItems::addButtonPanel(
		const QString & i_label,
		const QString & i_name,
		const QString & i_description,
		const QString & i_hotkey,
		bool i_dblclick)
{
	ButtonPanel * bp = new ButtonPanel( this, i_label, i_name, i_description, i_hotkey, i_dblclick);

	m_panel_l->addWidget( bp);

	m_btns.push_back( bp);

	return bp;
}

void ListItems::keyPressEvent( QKeyEvent * i_evt)
{
	if( i_evt->modifiers() & ( Qt::ControlModifier | Qt::AltModifier ))
		return;

	const QString str( i_evt->text());
	if( str.isNull() || str.isEmpty() )
		return;
//printf("ListItems::keyPressEvent: '%s'\n", str.toUtf8().data());

	if( ButtonPanel::setHotkey( str))
		return;

	for( int i = 0; i < m_btns.size(); i++ )
		m_btns[i]->keyPressed( str);
}

