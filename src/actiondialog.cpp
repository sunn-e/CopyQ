#include <QToolTip>
#include <QSettings>
#include <QFile>
#include <QCompleter>
#include <QDebug>
#include "actiondialog.h"
#include "action.h"
#include "ui_actiondialog.h"

static const QRegExp str("([^\\%])%s");

ActionDialog::ActionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActionDialog), m_completer(NULL)
{
    ui->setupUi(this);
    ui->inputText->clear();
    restoreHistory();
}

ActionDialog::~ActionDialog()
{
    saveHistory();
    delete ui;
}

void ActionDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ActionDialog::setInput(const QString &input)
{
    if ( input.isEmpty() ) {
        // no input
        ui->inputText->clear();
        ui->inputCheckBox->hide();
        ui->inputInfoLabel->hide();
        ui->inputText->hide();
    }
    else {
        ui->inputText->setText(input);
        ui->inputCheckBox->show();
        ui->inputInfoLabel->show();
        ui->inputText->show();
    }
}

void ActionDialog::add(const QString &cmd)
{
    if ( m_history.contains(cmd) )
        return;

    m_history.prepend(cmd);
    if ( m_history.size() > m_maxitems )
        m_history.removeLast();

    if ( m_completer )
        delete m_completer;
    m_completer = new QCompleter(m_history, this);
    ui->cmdEdit->setCompleter(m_completer);
}

void ActionDialog::restoreHistory()
{
    QSettings settings;

    m_maxitems = settings.value("command_history", 100).toInt();

    QFile file( dataFilename() );
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    QVariant v;
    while( !in.atEnd() ) {
        in >> v;
        m_history.append( v.toString() );
    }

    if ( m_completer )
        delete m_completer;
    m_completer = new QCompleter(m_history, this);
    ui->cmdEdit->setCompleter(m_completer);
}

const QString ActionDialog::dataFilename() const
{
    // do not use registry in windows
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    // .ini -> _cmds.dat
    QString datfilename = settings.fileName();
    datfilename.replace( QRegExp("\\.ini$"),QString("_cmds.dat") );

    return datfilename;
}

void ActionDialog::saveHistory()
{
    QFile file( dataFilename() );
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    QStringList::const_iterator it;
    for( it = m_history.begin(); it != m_history.end(); ++it)
        out << QVariant(*it);
}

void ActionDialog::closeAction(Action *act)
{
    const QString &err = act->getError();

    if ( !err.isEmpty() )
        emit error(err);

    disconnect(act);
    delete act;
}

void ActionDialog::accept()
{
    QString cmd = ui->cmdEdit->text();
    if ( cmd.isEmpty() )
        return;
    QStringList items;
    QString input = ui->inputText->text();
    Action *act;
    QString errstr;

    // replace %s with input
    if ( cmd.indexOf(str) != -1 ) {
        // if %s present and no input specified
        // TODO: warn user
        if ( input.isEmpty() )
            return;
        cmd.replace( str, "\\1"+input );
    }
    if ( !ui->inputCheckBox->isEnabled() )
        input.clear();

    // TODO: enable user to kill the process
    act = new Action( cmd, input.toLocal8Bit(),
                      ui->outputCheckBox->isChecked(),
                      ui->separatorEdit->text() );
    connect( act, SIGNAL(actionError(QString)),
             SIGNAL(error(QString)) );
    connect( act, SIGNAL(actionFinished(Action*)),
             this, SLOT(closeAction(Action*)) );
    connect( act, SIGNAL(newItems(QStringList)),
             SIGNAL(addItems(QStringList)) );

    add( ui->cmdEdit->text() );

    close();
}

void ActionDialog::setCommand(const QString &cmd)
{
    ui->cmdEdit->setText(cmd);
}

void ActionDialog::setSeparator(const QString &sep)
{
    ui->separatorEdit->setText(sep);
}

void ActionDialog::setInput(bool value)
{
    ui->inputCheckBox->setCheckState(value ? Qt::Checked : Qt::Unchecked);
}

void ActionDialog::setOutput(bool value)
{
    ui->inputCheckBox->setCheckState(value ? Qt::Checked : Qt::Unchecked);
}
