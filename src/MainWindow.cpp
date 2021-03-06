// MainWindow.cpp
// Generated by Interface Elements (Window v2.1) on Jun 27 1998
// This is a user written class and will not be overwritten.

#include "MainWindow.h"
#include "ThreadItem.h"
#include "TeamListView.h"
#include <stdio.h>
#include <InterfaceKit.h>
#include <stdlib.h>
#include "BetterScrollView.h"
#include "SettingsWindow.h"
#include "SlayerApp.h"
#include "AboutWindow.h"

MainWindow::MainWindow(void)
	: IEWindow("MainWindow")
{
	slayer->mainWindow = this;
	refreshThread = NULL;
	if (Lock()) {
		BView *mainBack = FindView("MainBack");
		BBox *scrollBox = (BBox *)FindView("MainTeamScrollerBox");
		CLVContainerView *scrollView;
		teamView = new TeamListView(scrollBox->Frame(), "MainTeamList", &scrollView);
		mainBack->RemoveChild(scrollBox);
		mainBack->AddChild(scrollView);
	
		team_items_list = 0;
		team_amount = 0;
		iteration = 0;

		refreshThread = new RefreshThread();		
		UpdateTeams();
		SetButtonState();

		if (slayer->options.wind_rect.IsValid()) {
			MoveTo(slayer->options.wind_rect.left,
			       slayer->options.wind_rect.top);
			ResizeTo(slayer->options.wind_rect.Width(),
			         slayer->options.wind_rect.Height());
		}
		minimized = false;
		if (slayer->options.workspace_activation == Options::all_workspaces)
			SetWorkspaces(B_ALL_WORKSPACES);
		else if (slayer->options.workspace_activation == Options::saved_workspace)
			SetWorkspaces(slayer->options.workspaces);
			
		if (slayer->options.wind_minimized)
			Minimize(true);
					
		// Quitting has to be disabled if docked
		if (slayer->docked) {
			BMenu *menu = (BMenu *)FindView("MainMenu");
			BMenuItem *item = menu->FindItem(IE_MAINWINDOW_MAINMENU_FILE_QUIT);
			item->SetEnabled(false);
		}

		// Set columns
		SetColumn(slayer->options.shown_columns);

		// Menu color hack to go around archiving bug(?) 
		FixArchive();
		
		refreshThread->Go();
		Unlock();
	}
	Show();
}


MainWindow::~MainWindow(void)
{
	slayer->mainWindow = NULL;
	refreshThread->Kill();
	delete refreshThread;
	if (!slayer->docked)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

void MainWindow::AttachedToWindow()
{
}

// Handling of user interface and other events
void MainWindow::MessageReceived(BMessage *message)
{

	switch(message->what){
		case TEAM_INV:
			BListItem *gItem;
			gItem = teamView->ItemAt(message->FindInt32("index"));
			break;
		case SELECTION_CHANGED:
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_KILL:
		case IE_MAINWINDOW_MAINKILL:
			DoKill();
		 	UpdateTeams();
		 	SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_SUSPEND:
		case IE_MAINWINDOW_MAINSUSPEND:
			DoSuspend();
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_RESUME:
		case IE_MAINWINDOW_MAINRESUME:
			DoResume();
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_LOW_PRIORITY:
			DoPriority(B_LOW_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_NORMAL_PRIORITY:
			DoPriority(B_NORMAL_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_DISPLAY_PRIORITY:
			DoPriority(B_DISPLAY_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_REAL_TIME_DISPLAY_PRIORITY:
			DoPriority(B_REAL_TIME_DISPLAY_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_URGENT_PRIORITY:
			DoPriority(B_URGENT_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYFIELD_REAL_TIME_PRIORITY:
			DoPriority(B_REAL_TIME_PRIORITY);
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINPRIORITYVALUE:
			// takes priority from text field
			DoPriority();
			UpdateTeams();
			SetButtonState();
			break;			
		case IE_MAINWINDOW_MAINUPDATE:
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_FILE_ABOUT_SLAYER___:    // "About…" is selected from menu…
		{
			BWindow *about = slayer->FindWindow("About Slayer");
			if (!about)
				new AboutWindow();
			else if (about->Lock()) {
				about->Activate(true);
				about->Unlock();
			}
		}
			break;
		case IE_MAINWINDOW_MAINMENU_FILE_QUIT:    // "Quit" is selected from menu…
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case IE_MAINWINDOW_MAINMENU_EDIT_CUT:    // "Cut" is selected from menu…
			break;

		case IE_MAINWINDOW_MAINMENU_EDIT_COPY:    // "Copy" is selected from menu…
			break;

		case IE_MAINWINDOW_MAINMENU_EDIT_PASTE:    // "Paste" is selected from menu…
			break;
		
		case IE_MAINWINDOW_MAINMENU_COLUMNS_ID:
			SwitchColumn(Options::id_col, message->what);
			break;
		case IE_MAINWINDOW_MAINMENU_COLUMNS_PRIORITY:
			SwitchColumn(Options::priority_col, message->what);
			break;
		case IE_MAINWINDOW_MAINMENU_COLUMNS_STATE:
			SwitchColumn(Options::state_col, message->what);
			break;
		case IE_MAINWINDOW_MAINMENU_COLUMNS_MEMORY:
			SwitchColumn(Options::memory_col, message->what);
			break;
		case IE_MAINWINDOW_MAINMENU_COLUMNS_CPU:
			SwitchColumn(Options::cpu_col, message->what);
			break;

		case IE_MAINWINDOW_MAINMENU_WINDOWS_SETTINGS___:
		{
			BWindow *settings = slayer->FindWindow("Settings");
			if (!settings)
				new SettingsWindow();
			else if (settings->Lock()) {
				settings->Activate(true);
				settings->Unlock();
			}
		}
			break;
		default:
			IEWindow::MessageReceived(message);
			break;
	}

}


void MainWindow::Quit()
{

	if (slayer->options.save_wind_pos) {
		slayer->options.wind_rect = Frame();
		slayer->options.wind_minimized = minimized;
	}
	if (slayer->options.save_workspace)
		slayer->options.workspaces = Workspaces();

	// What follows is a really ugly hack to detect if the user closed
	// the window with close button, or if Application wants to close
	// all windows:
	BMessage *msg = CurrentMessage();  // this is null if called outside BMessageReceived loop
	                                   // -> message from application 
	if (slayer->docked && msg != NULL)
		Minimize(true);
	else
		IEWindow::Quit();
}

void MainWindow::Minimize(bool minimize)
{
	minimized = minimize;
	IEWindow::Minimize(minimize);
	
	// if window is minimized, remove update thread. If window un-minimized, restart it.
	if (minimized && refreshThread != NULL) {
		// Gotta get rid of the refresh thread
		refreshThread->Kill();
	}
	else if (!minimized && refreshThread != NULL) {
		refreshThread->Go();
	}
}

// unfortunately Minimize isn't called when the window is un-minimized,
// so here is a dirty hack around it.
void MainWindow::WindowActivated(bool active)
{
	if (active) {
		if (minimized) {
			minimized = false;
			// Update the teams
			UpdateTeams();
			// restart updating
			if (refreshThread != NULL) refreshThread->Go();
		}
	}
}
void MainWindow::UpdateTeams()
{
	ThreadItem *thread_item;
	TeamItem   *team_item;

	DisableUpdates();

	if (!team_items_list) 
		team_items_list = new Hashtable;

	app_info inf;
	thread_info thinf;
	int32 th_cookie, te_cookie;
	int32 i;
	
	team_info teinf;
	
	te_cookie = 0;

	iteration = (iteration + 1) % 2;

	total_CPU_diff = 0;
	int idle_CPU_diff = 0;
		
	for (i = 0; get_next_team_info(&te_cookie, &teinf) == B_NO_ERROR; i++) {
		if (!(team_item = (TeamItem *)team_items_list->get(teinf.team))) {
			team_item = new TeamItem(&teinf);
			team_item->refreshed = iteration;
			team_item->SetExpanded(false);
			team_items_list->put(teinf.team, team_item);
			teamView->AddItem(team_item);
				
			team_item->CPU_diff = 0;
			for (th_cookie = 0; get_next_thread_info(team_item->team, &th_cookie, &thinf) == B_OK;) {
				thread_item = new ThreadItem(&thinf);
				thread_item->refreshed = iteration;
				team_item->thread_items_list->put(thinf.thread, thread_item);
				teamView->AddUnder(thread_item, team_item);
				if (teinf.team != 1 || strncmp(thinf.name, "idle thread ", 12) != 0) {
					team_item->CPU_diff += thread_item->CPU_diff;
				} else
					idle_CPU_diff += thread_item->CPU_diff;
			}
		}
		// update team
		else {
			team_item->update(&teinf);
			team_item->refreshed = iteration;
		
			team_item->CPU_diff = 0;	
			for (th_cookie = 0; get_next_thread_info(team_item->team, &th_cookie, &thinf) == B_OK;) {
				if (!(thread_item = (ThreadItem *)team_item->thread_items_list->get(thinf.thread))) {
					thread_item = new ThreadItem(&thinf);
					thread_item->refreshed = iteration;
					team_item->thread_items_list->put(thinf.thread, thread_item);
					teamView->AddUnder(thread_item, team_item);
				}
				// update thread
				else {
					thread_item->update(&thinf);
					thread_item->refreshed = iteration;
				}
				if (teinf.team != 1 || strncmp(thinf.name, "idle thread ", 12) != 0) {
					team_item->CPU_diff += thread_item->CPU_diff;
				} else
					idle_CPU_diff += thread_item->CPU_diff;
			} 

		}
		total_CPU_diff += team_item->CPU_diff;
		if (total_CPU_diff < 0) printf("Error. CPU diff out of bounds\n");
	}
	total_CPU_diff += idle_CPU_diff;

	// division by zero && overflow handling
	if (total_CPU_diff <= 0) total_CPU_diff = 1;

	RemoveList.MakeEmpty();
	teamView->FullListDoForEach(postlistproc, (void *)this);
	RemoveProcessItems(&RemoveList);

	EnableUpdates();
}

void MainWindow::RemoveProcessItems(BList *items) 
{
	CLVListItem **p = (CLVListItem **)items->Items();
	int32 i;
	for (i = 0; i < items->CountItems(); i++) {
		teamView->RemoveItem(p[i]);
		if (!p[i]->OutlineLevel()) {
			team_items_list->del(((TeamItem *)p[i])->team);
			delete ((TeamItem *)p[i]);
		}
		else {
			// find which team this belongs to
			TeamItem *team_item = (TeamItem *)team_items_list->get(((ThreadItem *)p[i])->team);
			// can be null if the team is already taken away
			if (team_item != NULL)
				team_item->thread_items_list->del(((ThreadItem *)p[i])->thread);
		
			delete ((ThreadItem *)p[i]);
		}
	}
}


bool postlistproc(CLVListItem *item, void *_wnd)
{
	MainWindow *wnd = (MainWindow *)_wnd;	
	if (!item->OutlineLevel()) {
		if (((TeamItem *)item)->refreshed != wnd->iteration)
			wnd->RemoveList.AddItem((void *)item);
		else {
			int32 ch = ((TeamItem *)item)->changed;
			while (ch) {
				BRect rect(0.0, 0.0, 0.0, 0.0);
				if (ch & TeamItem::name_chg) {
					rect = item->ItemColumnFrame(TeamListView::name_ndx, wnd->teamView);
					ch &= ~(TeamItem::name_chg);
				}
				else if (ch & TeamItem::areas_chg) {
					rect = item->ItemColumnFrame(TeamListView::areas_ndx, wnd->teamView);
					ch &= ~(TeamItem::areas_chg);
				}
				else ch = 0;
				if (rect.right && rect.bottom) wnd->teamView->Invalidate(rect);
			}
			((TeamItem *)item)->changed = 0;
			
			float CPU = ((float)((TeamItem *)item)->CPU_diff) / wnd->total_CPU_diff;
			if ((CPU != ((TeamItem *)item)->CPU) &&
			    (slayer->options.shown_columns & Options::cpu_col)) {
	
				CPU = (CPU > 1.0 ? 1.0 : CPU < 0.0 ? 0.0 : CPU);
				((TeamItem *)item)->CPU = CPU;
				item->DrawItemColumn(wnd->teamView, item->ItemColumnFrame(
					TeamListView::CPU_ndx, wnd->teamView), TeamListView::CPU_ndx, true);
			} 
		}
	}
	else {
		if (((ThreadItem *)item)->refreshed != wnd->iteration)
			wnd->RemoveList.AddItem((void *)item);
		else {
			int32 ch = ((ThreadItem *)item)->changed;
			while (ch) {
				BRect rect(0.0, 0.0, 0.0, 0.0);
				if (ch & ThreadItem::name_chg) {
					rect = item->ItemColumnFrame(TeamListView::name_ndx, wnd->teamView);
					ch &= ~(ThreadItem::name_chg);
				}
				else if (ch & ThreadItem::priority_chg) {
					rect = item->ItemColumnFrame(TeamListView::priority_ndx, wnd->teamView);
					ch &= ~(ThreadItem::priority_chg);
				}
				else if (ch & ThreadItem::state_chg) {
					rect = item->ItemColumnFrame(TeamListView::state_ndx, wnd->teamView);
					ch &= ~(ThreadItem::state_chg);
				}
				else ch = 0;
				
				if (rect.right && rect.bottom) wnd->teamView->Invalidate(rect);
			}
			((ThreadItem *)item)->changed = 0;

			float CPU = ((float)((ThreadItem *)item)->CPU_diff) / wnd->total_CPU_diff;
			if ((CPU != ((ThreadItem *)item)->CPU) &&
			    (slayer->options.shown_columns & Options::cpu_col)) {

				CPU = (CPU > 1.0 ? 1.0 : CPU < 0.0 ? 0.0 : CPU);
				((ThreadItem *)item)->CPU = CPU;
				item->DrawItemColumn(wnd->teamView, item->ItemColumnFrame(
					TeamListView::CPU_ndx, wnd->teamView), TeamListView::CPU_ndx, true);
			}
			
		}
	}
	return false;
}

void MainWindow::DoKill(void) 
{
	BListItem 	*gItem;
	int32		selected, i = 0;
	
	for (; (selected = teamView->CurrentSelection(i)) >= 0; i++) {
		// is a team or thread?
		gItem = teamView->ItemAt(selected);
		if (!gItem->OutlineLevel())
			kill_team(((TeamItem *)gItem)->team);
		else
			kill_thread(((ThreadItem *)gItem)->thread);
	}
}

void MainWindow::DoPriority(int32 priority)
{
	// BListItem 	*gItem;
	CLVListItem	*gItem;
	
	int32		selected, i = 0;

	for (; (selected = teamView->CurrentSelection(i)) >= 0; i++) {
		// is a team or thread?
		gItem = (CLVListItem *)teamView->ItemAt(selected);
		if (!gItem->OutlineLevel()) {
			// this is for normal BOutLineListView
			//int32 ir = 0, iu = teamView->CountItemsUnder(gItem, true);
			// this is for ColumnListView
			int32 ir = teamView->FullListIndexOf(gItem) + 1,
				iu = teamView->FullListNumberOfSubitems(gItem) + ir;
				
			for (; ir < iu; ir++)
				set_thread_priority(((ThreadItem *)teamView->FullListItemAt(ir))->thread,
				                    priority);			    
		}
		else
			set_thread_priority(((ThreadItem *)gItem)->thread, priority);
	}
}

void MainWindow::DoPriority()
{
	BTextControl *PriorityValue = (BTextControl *)FindView("MainPriorityValue");
	if (strcmp("", PriorityValue->Text())) { 
		int32 value;
		value = atoi(PriorityValue->Text());
		DoPriority(value);
	}
}

void MainWindow::DoSuspend(void)
{
	//BListItem 	*gItem;
	CLVListItem	*gItem;
	int32		selected, i = 0;

	for (; (selected = teamView->CurrentSelection(i)) >= 0; i++) {
		// is a team or thread?
		gItem = (CLVListItem*)teamView->ItemAt(selected);
		if (!gItem->OutlineLevel()) {
//			int32 ir = 0, iu = teamView->CountItemsUnder(gItem, true);
			int32 ir = teamView->FullListIndexOf(gItem) + 1,
				iu = teamView->FullListNumberOfSubitems(gItem) + ir;

			for (; ir < iu; ir++)
				suspend_thread(((ThreadItem *)teamView->FullListItemAt(ir))->thread);
		}
		else
			suspend_thread(((ThreadItem *)gItem)->thread);
	}
}

void MainWindow::DoResume(void)
{
	//BListItem 	*gItem;
	CLVListItem	*gItem;
	int32		selected, i = 0;

	for (; (selected = teamView->CurrentSelection(i)) >= 0; i++) {
		// is a team or thread?
		gItem = (CLVListItem *)teamView->ItemAt(selected);
		if (!gItem->OutlineLevel()) {
//			int32 ir = 0, iu = teamView->CountItemsUnder(gItem, true);
			int32 ir = teamView->FullListIndexOf(gItem) + 1,
				iu = teamView->FullListNumberOfSubitems(gItem) + ir;

			for (; ir < iu; ir++)
				resume_thread(((ThreadItem *)teamView->FullListItemAt(ir))->thread);
		}
		else
			resume_thread(((ThreadItem *)gItem)->thread);
	}

}

void MainWindow::SetButtonState()
{
	BButton *Kill = (BButton *)FindView("MainKill");
	BButton *Suspend = (BButton *)FindView("MainSuspend");
	BButton *Resume = (BButton *)FindView("MainResume");
	int32 sel = teamView->CurrentSelection();
	bool is_sel = (sel >= 0 ? true : false);
	
	Kill->SetEnabled(is_sel);
	Suspend->SetEnabled(is_sel);
	Resume->SetEnabled(is_sel);

	BMenu *menu = (BMenu *)FindView("MainMenu");
	BMenuItem *item = menu->FindItem(IE_MAINWINDOW_MAINMENU_ACTION_KILL);
	if (item) item->SetEnabled(is_sel);
	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_ACTION_SUSPEND);
	if (item) item->SetEnabled(is_sel);
	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_ACTION_RESUME);
	if (item) item->SetEnabled(is_sel);
		
	SetPriorityState();
}

void MainWindow::SetPriorityState()
{
	BTextControl *PriorityValue = (BTextControl *)FindView("MainPriorityValue");
	BMenuField *Priority = (BMenuField *)FindView("MainPriorityField");
	int32 sel = teamView->CurrentSelection();

	Priority->SetEnabled((sel >= 0 ? true : false));
	PriorityValue->SetEnabled((sel >= 0 ? true : false));
	
	if (sel >= 0) {
		BListItem *gItem = teamView->ItemAt(sel);
		int32 priority;
		BMenuItem *it;
		
		// if single thread selected	
		if (gItem->OutlineLevel() && teamView->CurrentSelection(1) < 0) {
			char pr_text[10] = "";
			priority = ((ThreadItem *)gItem)->priority;
			sprintf(pr_text, "%ld", priority);
			// set only if the new value is different from the old
			if (strcmp(pr_text, PriorityValue->Text()))
				PriorityValue->SetText(pr_text);
		
			SetPriorityField(priority);	
		}
		else if ((it = Priority->Menu()->ItemAt(0))) {
			if (strcmp("", PriorityValue->Text()))
				// assume the first item is "Select"
				PriorityValue->SetText("");

			if (!it->IsMarked()) it->SetMarked(true);
				
		}
	}	
}

void MainWindow::SetPriorityField(int32 priority)
{
	BMenuField *Priority = (BMenuField *)FindView("MainPriorityField");

	int32 c = -1;
	BMenuItem *it;
	
	if      (priority < B_NORMAL_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_LOW_PRIORITY;
	else if (priority < B_DISPLAY_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_NORMAL_PRIORITY;
	else if (priority < B_REAL_TIME_DISPLAY_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_DISPLAY_PRIORITY;
	else if (priority < B_URGENT_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_REAL_TIME_DISPLAY_PRIORITY;	
	else if (priority < B_REAL_TIME_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_URGENT_PRIORITY;
	else if (priority >= B_REAL_TIME_PRIORITY)
		c = IE_MAINWINDOW_MAINPRIORITYFIELD_REAL_TIME_PRIORITY;
	
	it = Priority->Menu()->FindItem(c);
	if (it && !it->IsMarked())
		it->SetMarked(true);
}

void MainWindow::SwitchColumn(int32 col_id, int32 menuid)
{
	BMenu *menu = (BMenu *)FindView("MainMenu");
	BMenuItem *item = menu->FindItem(menuid);
	bool marked = item->IsMarked();
	
	if (marked)
		slayer->options.shown_columns &= ~col_id;
	else
		slayer->options.shown_columns |= col_id;

//	slayer->options.shown_columns = (marked ? (slayer->options.shown_columns & ~col_id : slayer->options.shown_columns | col_id));
	item->SetMarked(marked ? false : true);
	
	teamView->SetShownColumns(slayer->options.shown_columns);
	
	if (!marked) UpdateTeams();
}

void MainWindow::SetColumn(int32 col_mask)
{
	slayer->options.shown_columns = col_mask;

	BMenu *menu = (BMenu *)FindView("MainMenu");
	BMenuItem *item;

	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_COLUMNS_ID);
	item->SetMarked(col_mask & Options::id_col ? true : false);

	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_COLUMNS_PRIORITY);
	item->SetMarked(col_mask & Options::priority_col ? true : false);

	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_COLUMNS_STATE);
	item->SetMarked(col_mask & Options::state_col ? true : false);

	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_COLUMNS_MEMORY);
	item->SetMarked(col_mask & Options::memory_col ? true : false);

	item = menu->FindItem(IE_MAINWINDOW_MAINMENU_COLUMNS_CPU);
	item->SetMarked(col_mask & Options::cpu_col ? true : false);

	teamView->SetShownColumns(slayer->options.shown_columns);		
}

void fix_menu(BMenu *menu, menu_info *mi) 
{
	int32 items = menu->CountItems();
	int32 run = 0;
	BMenu *sub;

	menu->SetViewColor(mi->background_color);
		
	for (; run < items; run++) 
		if ((sub = menu->SubmenuAt(run)) != NULL)
			fix_menu(sub, mi);
}

void MainWindow::FixArchive()
{
	menu_info mi;
	get_menu_info(&mi);
	
	BMenu *menu = (BMenu *)FindView("MainMenu");
	fix_menu(menu, &mi);
	
	BMenuField *Priority = (BMenuField *)FindView("MainPriorityField");
	fix_menu(Priority->Menu(), &mi); // ->SetViewColor(mi.background_color);
	Priority->MenuBar()->SetViewColor(mi.background_color);
}

